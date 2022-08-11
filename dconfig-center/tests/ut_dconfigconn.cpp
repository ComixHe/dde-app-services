// SPDX-FileCopyrightText: 2021 - 2022 Uniontech Software Technology Co.,Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <QBuffer>
#include <QFile>
#include <QLocale>
#include <QSignalSpy>
#include <QDir>

#include <gtest/gtest.h>

#include "dconfigresource.h"
#include "dconfigconn.h"
#include "test_helper.hpp"

static constexpr char const *LocalPrefix = "/tmp/example/";
static constexpr char const *APP_ID = "org.foo.appid";
static constexpr char const *FILE_NAME = "example";

static QString configPath()
{
    const QString metaPath = QString("%1/usr/share/dsg/configs/%2").arg(LocalPrefix, APP_ID);
    return QString("%1/%2.json").arg(metaPath, FILE_NAME);
}

static EnvGuard dsgDataDir;
class ut_DConfigResource : public testing::Test
{
protected:
    static void SetUpTestCase() {
        auto path = configPath();
        if (!QFile::exists(path)) {
            QDir("").mkpath(QFileInfo(path).path());
        }

        ASSERT_TRUE(QFile::copy(":/config/example.json", path));
        qputenv("DSG_CONFIG_CONNECTION_DISABLE_DBUS", "true");
        dsgDataDir.set("DSG_DATA_DIRS", "/usr/share/dsg");
    }
    static void TearDownTestCase() {
        QFile::remove(configPath());
        qunsetenv("DSG_CONFIG_CONNECTION_DISABLE_DBUS");
        dsgDataDir.restore();
    }
    virtual void SetUp() override {
        resource.reset(new DSGConfigResource("/example", LocalPrefix));
    }
    virtual void TearDown() override {

    }

    QScopedPointer<DSGConfigResource> resource;
};

TEST_F(ut_DConfigResource, load) {

    ASSERT_TRUE(resource->load(APP_ID, FILE_NAME, ""));
}
TEST_F(ut_DConfigResource, load_fail) {

    ASSERT_FALSE(resource->load(APP_ID, "example_notexist", ""));
}
TEST_F(ut_DConfigResource, createConn) {

    resource->load(APP_ID, FILE_NAME, "");
    ASSERT_TRUE(resource->createConn(0));
}


class ut_DConfigConn : public testing::Test
{
protected:
    static void SetUpTestCase() {
        auto path = configPath();
        if (!QFile::exists(path)) {
            QDir("").mkpath(QFileInfo(path).path());
        }

        ASSERT_TRUE(QFile::copy(":/config/example.json", path));
        qputenv("DSG_CONFIG_CONNECTION_DISABLE_DBUS", "true");
        dsgDataDir.set("DSG_DATA_DIRS", "/usr/share/dsg");
    }
    static void TearDownTestCase() {
        QFile::remove(configPath());
        QDir(LocalPrefix).removeRecursively();
        qunsetenv("DSG_CONFIG_CONNECTION_DISABLE_DBUS");
        dsgDataDir.restore();
    }
    virtual void SetUp() override {
        resource.reset(new DSGConfigResource("/example", LocalPrefix));
        resource->load(APP_ID, FILE_NAME, "");
        conn = resource->createConn(0);
        ASSERT_TRUE(conn);
    }
    virtual void TearDown() override {

    }
    DSGConfigConn* conn;
    QScopedPointer<DSGConfigResource> resource;
};


TEST_F(ut_DConfigConn, description_name) {
    ASSERT_EQ(conn->description("canExit", ""), "我是描述");
    ASSERT_EQ(conn->description("canExit", "en_US"), "I am description");
    ASSERT_EQ(conn->name("canExit", ""), "I am name");
    ASSERT_EQ(conn->name("canExit", "zh_CN"), QString("我是名字"));
    ASSERT_EQ(conn->name("canExit", QLocale(QLocale::Japanese).name()), "");
    ASSERT_EQ(conn->flags("canExit"), 0);
}

TEST_F(ut_DConfigConn, value) {
    conn->setValue("canExit", QDBusVariant{true});
    ASSERT_EQ(conn->value("canExit").variant(), true);
    const QStringList array{"value1", "value2"};
    QVariantMap map;
    map.insert("key1", "value1");
    map.insert("key2", "value2");
    ASSERT_EQ(conn->value("array").variant().toStringList(), array);
    ASSERT_EQ(conn->value("map").variant().toMap(), map);

    QVariantMap mapArray;
    mapArray.insert("key1", QStringList{"value1"});
    mapArray.insert("key2", QStringList{"value2"});
    QVariantList arrayMap;
    arrayMap.append(map);
    ASSERT_EQ(conn->value("map_array").variant().toMap(), mapArray);
    ASSERT_EQ(conn->value("array_map").variant().toList(), arrayMap);
}

TEST_F(ut_DConfigConn, setValue) {
    conn->setValue("canExit", QDBusVariant{false});

    QSignalSpy spy(conn, &DSGConfigConn::valueChanged);
    conn->setValue("canExit", QDBusVariant{true});
    ASSERT_EQ(conn->value("canExit").variant(), true);
    ASSERT_EQ(spy.count(), 1);
}

TEST_F(ut_DConfigConn, reset) {
    conn->setValue("canExit", QDBusVariant{false});

    QSignalSpy spy(conn, &DSGConfigConn::valueChanged);
    conn->reset("canExit");
    ASSERT_EQ(conn->value("canExit").variant(), true);
    ASSERT_EQ(spy.count(), 1);
}

TEST_F(ut_DConfigConn, visibility) {

    ASSERT_EQ(conn->visibility("canExit"), "private");
}

TEST_F(ut_DConfigConn, release) {
    QSignalSpy spy(conn, &DSGConfigConn::releaseChanged);
    ASSERT_EQ(spy.count(), 0);

    conn->release();

    ASSERT_EQ(spy.count(), 1);
    conn->release();
}
