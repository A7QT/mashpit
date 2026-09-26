// mashpit app v1 entry point.
#include <QApplication>
#include <QMessageBox>
#include <cstdio>

#include "control/controlobject.h"
#include "deck_engine.h"
#include "main_window.h"

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    app.setApplicationName("mashpit");
    app.setOrganizationName("mashpit");

    // Headless end-to-end check (no audio device needed):
    //   mashpit-app --selftest song.wav
    // Loads deck A, plays 200 blocks, prints peak. Exit 0 = engine alive.
    if (argc == 3 && QString(argv[1]) == "--selftest") {
        DeckEngine engine;
        const QString err = engine.loadFile(0, QString::fromLocal8Bit(argv[2]));
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
