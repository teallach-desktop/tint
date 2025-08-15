// SPDX-License-Identifier: GPL-2.0-only
#include <signal.h>
#include <QApplication>
#include <QCommandLineParser>
#include "log.h"
#include "conf.h"
#include "panel.h"

void handleQuitSignals(const std::vector<int> &quitSignals)
{
    auto handler = [](int sig) -> void {
        Q_UNUSED(sig);
        QCoreApplication::quit();
    };
    for (int sig : quitSignals)
        signal(sig, handler);
}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    QApplication::setApplicationName("tint");
    QApplication::setApplicationVersion("0.0.0");
    handleQuitSignals({ SIGQUIT, SIGINT, SIGTERM, SIGHUP });

    QCommandLineParser parser;
    parser.setApplicationDescription("A Wayland panel inspired by tint2");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption debug(QStringList() << "d" << "debug");
    debug.setDescription("Enable full logging, including debug information");
    parser.addOption(debug);

    QCommandLineOption output(QStringList() << "o" << "output");
    output.setDescription("Output to use");
    output.setValueName("output");
    parser.addOption(output);

    QCommandLineOption config(QStringList() << "c" << "config");
    config.setDescription("Config file to use");
    config.setValueName("filename");
    parser.addOption(config);

    parser.process(app);

    QString filename = parser.value(config);
    if (filename.isEmpty()) {
        filename = qgetenv("HOME") + "/.config/tint/tintrc";
    }

    info("read config file '{}'", filename.toStdString());
    confInit(filename);
    confSetOutput(parser.value(output));
    if (parser.isSet(debug)) {
        confSetVerbosity(1);
    }

    Panel panel;
    panel.show();
    return app.exec();
}
