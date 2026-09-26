// mashpit app v1 entry point.
#include <QApplication>
#include <QMessageBox>
#include <QPixmap>
#include <QThread>
#include <cstdio>

#include "control/controlobject.h"
#include "deck_engine.h"
#include "main_window.h"

namespace {

int runSelftest(DeckEngine& engine, const QString& file) {
    const QString err = engine.loadFile(0, file);
    if (!err.isEmpty()) {
        fprintf(stderr, "selftest LOAD FAIL: %s\n", err.toLocal8Bit().constData());
        return 3;
    }
    ControlObject::set(ConfigKey(QStringLiteral("[Channel1]"), QStringLiteral("play")), 1.0);
    double peak = 0.0;
    for (int i = 0; i < 200; ++i) {
        peak = std::max(peak, engine.processForTest(512));
    }
    fprintf(stderr, "selftest peak=%.4f %s\n", peak, peak > 1e-6 ? "OK" : "SILENCE-FAIL");
    return peak > 1e-6 ? 0 : 4;
}

int runScreenshot(DeckEngine& engine, const QString& path, const QString& arrangement) {
    MainWindow win(&engine);
    if (!arrangement.isEmpty()) {
        win.openArrangementPath(arrangement);
    }
    win.show();
    QApplication::processEvents();
    QThread::msleep(400);
    QApplication::processEvents();
    const QPixmap shot = win.grab();
    if (!shot.save(path)) {
        fprintf(stderr, "screenshot SAVE FAIL\n");
        return 5;
    }
    fprintf(stderr, "screenshot saved\n");
    return 0;
}

} // namespace

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    app.setApplicationName("mashpit");
    app.setOrganizationName("mashpit");

    // Headless end-to-end check (no audio device needed):
    //   mashpit-app --selftest song.wav
    if (argc == 3 && QString(argv[1]) == "--selftest") {
        DeckEngine engine;
        return runSelftest(engine, QString::fromLocal8Bit(argv[2]));
    }

    // Layout verification for headless dev:
    //   mashpit-app --screenshot out.png [arrangement.json]
    if ((argc == 3 || argc == 4) && QString(argv[1]) == "--screenshot") {
        DeckEngine engine;
        return runScreenshot(engine, QString::fromLocal8Bit(argv[2]),
                argc == 4 ? QString::fromLocal8Bit(argv[3]) : QString());
    }

    DeckEngine engine;
    const QString audioErr = engine.startAudio();
    if (!audioErr.isEmpty()) {
        QMessageBox::critical(nullptr, "mashpit — no audio",
                QString("Could not open the default audio device:\n%1\n\n"
                        "The booth needs live output in v1. "
                        "Rendering still works via `mashpit render`.")
                        .arg(audioErr));
        return 1;
    }

    MainWindow win(&engine);
    win.show();
    return app.exec();
}
