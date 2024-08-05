#include <QCoreApplication>
#include <QWebSocketServer>
#include <QWebSocket>
#include <QJsonObject>
#include <QJsonDocument>
#include <QObject>
#include <QMetaProperty>
#include <QFile>
#include <QMap>
#include <memory>
#include <QDebug>

class DynamicObject : public QObject {
    Q_OBJECT
public:
    DynamicObject(QObject *parent = nullptr) : QObject(parent) {}

    void setProperty(const QString &name, const QVariant &value) {
        dynamicProperties[name] = value;
        qDebug() << "Property set:" << name << "=" << value;
    }

    QVariant getProperty(const QString &name) const {
        return dynamicProperties.value(name, QVariant());
    }

    QJsonObject toJson() const {
        QJsonObject obj;
        for (auto it = dynamicProperties.begin(); it != dynamicProperties.end(); ++it) {
            obj[it.key()] = QJsonValue::fromVariant(it.value());
        }
        return obj;
    }

private:
    QMap<QString, QVariant> dynamicProperties;
};

class ObjectManager : public QObject {
    Q_OBJECT
public:
    ObjectManager() : nextId(1) {}

    int createObject() {
        auto obj = std::make_shared<DynamicObject>();
        int id = nextId++;
        objects[id] = obj;
        qDebug() << "Object created with ID:" << id;
        return id;
    }

    std::shared_ptr<DynamicObject> getObject(int id) {
        return objects.value(id, nullptr);
    }

    bool deleteObject(int id) {
        bool removed = objects.remove(id) > 0;
        qDebug() << "Object deleted with ID:" << id << ", success:" << removed;
        return removed;
    }

    QJsonObject listObjects() const {
        QJsonObject result;
        for (auto it = objects.begin(); it != objects.end(); ++it) {
            result[QString::number(it.key())] = it.value()->toJson();
        }
        qDebug() << "Objects listed:" << result;
        return result;
    }

private:
    QMap<int, std::shared_ptr<DynamicObject>> objects;
    int nextId;
};

class WebSocketServer : public QObject {
    Q_OBJECT
public:
    WebSocketServer(quint16 port, QObject *parent = nullptr)
        : QObject(parent), server(new QWebSocketServer(QStringLiteral("JSON-RPC Server"),
                                      QWebSocketServer::NonSecureMode, this)) {
        if (server->listen(QHostAddress::Any, port)) {
            connect(server, &QWebSocketServer::newConnection, this, &WebSocketServer::onNewConnection);
            connect(server, &QWebSocketServer::closed, this, &WebSocketServer::closed);
            qDebug() << "WebSocket server listening on port" << port;
        }
    }

private slots:
    void onNewConnection() {
        QWebSocket *client = server->nextPendingConnection();
        connect(client, &QWebSocket::textMessageReceived, this, &WebSocketServer::processTextMessage);
        connect(client, &QWebSocket::disconnected, client, &QWebSocket::deleteLater);
        qDebug() << "New client connected.";
    }

    void processTextMessage(const QString &message) {
        QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
        QJsonObject request = doc.object();
        QJsonObject response;

        response["jsonrpc"] = "2.0";
        if (request.contains("id"))
            response["id"] = request["id"];

        QString method = request["method"].toString();
        QJsonObject params = request["params"].toObject();

        if (method == "createObject") {
            int id = objectManager.createObject();
            response["result"] = id;
        } else if (method == "deleteObject") {
            int id = params["id"].toInt();
            bool success = objectManager.deleteObject(id);
            response["result"] = success;
        } else if (method == "getObject") {
            int id = params["id"].toInt();
            auto obj = objectManager.getObject(id);
            response["result"] = obj ? obj->toJson() : QJsonObject();
        } else if (method == "listObjects") {
            response["result"] = objectManager.listObjects();
        } else {
            response["error"] = QJsonObject{{"code", -32601}, {"message", "Method not found"}};
        }

        QWebSocket *client = qobject_cast<QWebSocket *>(sender());
        client->sendTextMessage(QJsonDocument(response).toJson(QJsonDocument::Compact));
        qDebug() << "Sent response to client:" << QJsonDocument(response).toJson(QJsonDocument::Compact);
    }

signals:
    void closed();

private:
    QWebSocketServer *server;
    ObjectManager objectManager;
};

int main(int argc, char *argv[]) {
    QCoreApplication a(argc, argv);

    WebSocketServer server(12345);
    return a.exec();
}

#include "main.moc"
