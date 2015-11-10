// Copyright (c) 2001-2013, SCALABLE Network Technologies, Inc.  All Rights Reserved.
//                          600 Corporate Pointe
//                          Suite 1200
//                          Culver City, CA 90230
//                          info@scalable-networks.com
//
// This source code is licensed, not sold, and is subject to a written
// license agreement.  Among other things, no portion of this source
// code may be copied, transmitted, disclosed, displayed, distributed,
// translated, used as the basis for a derivative work, or used, in
// whole or in part, for any program or purpose other than its intended
// use in compliance with the license agreement as part of the EXata
// software.  This source code and certain of the algorithms contained
// within it are confidential trade secrets of Scalable Network
// Technologies, Inc. and may not be used as the basis for any other
// software, hardware, product or service.


#include "Extractor.h"
#include "Welcome.h"
#include "Setup.h"
#include "AdjustNames.h"
#include "AdjustSubnets.h"
#include "DebugWindow.h"

#include "configFiles.h"
#include "Config.h"
#include "vrlink.h"

#include <QSplashScreen>
#include <QFileDialog>
#include <QMessageBox>
#include <QDesktopServices>
#include <QTimer>

#ifdef LINK_STATIC_PLUGINS
#include <QtPlugin>
Q_IMPORT_PLUGIN(qjpeg)
Q_IMPORT_PLUGIN(qgif)
Q_IMPORT_PLUGIN(qtiff)
#endif

using namespace SNT_SYNCH;

typedef const char* charPtr;

void GradientFrame::paintEvent( QPaintEvent * event )
    {
        QPainter p(this);
        QLinearGradient fade(0,height(),width(),0);
        fade.setColorAt(0, QColor(0, 50, 75, 255));
        fade.setColorAt(1, QColor(100, 175, 255, 255));
        fade.setSpread(QGradient::ReflectSpread);
        p.fillRect(rect(), fade);
    }
void ShadowLabel::paintEvent( QPaintEvent * event )
{
    QStyle *style = QWidget::style();
    QPainter painter(this);
    QRect cr = contentsRect();
    int flags = QStyle::visualAlignment(layoutDirection(), QFlag(alignment()));
    if (wordWrap())
        flags = flags | Qt::TextWordWrap;
    QPalette pal = palette();
    QColor org = pal.color(QPalette::WindowText);
    pal.setColor(QPalette::WindowText, QColor(255-org.red(),255-org.green(), 255-org.blue(), 255));
    style->drawItemText(&painter, cr, flags, pal, isEnabled(), text(), foregroundRole());
    cr.moveTo(cr.x()+1, cr.y()+1);
    style->drawItemText(&painter, cr, flags, pal, isEnabled(), text(), foregroundRole());
    cr.moveTo(cr.x()-2, cr.y()-2);
    pal.setColor(QPalette::WindowText, org );
    style->drawItemText(&painter, cr, flags, pal, isEnabled(), text(), foregroundRole());
}

Extractor::Extractor(QApplication* application) : app(application), welcome(0),
                                                  vrlink(0), ns(0),
                                                  dbW(0), connectThread(0),
                                                  windowIcon(":/icon.png")
{
#ifdef _WIN32
    userHome = QString(getenv("HOMEDRIVE")) + QString(getenv("HOMEPATH")) + QString("\\My Documents");
#else
    userHome = QString(getenv("HOME"));
#endif
    exeHome = SNT_SYNCH::Config::instance().exeHome.c_str();
    char currentWorkingDirectory[FILENAME_MAX];
    std::string cwdStr;
    memset(currentWorkingDirectory, 0, FILENAME_MAX);
    if (GetCurrentDir(currentWorkingDirectory, FILENAME_MAX))
    {
        cwdStr = currentWorkingDirectory;  //This will should always be /HOME/gui/lib/<platform>
                
#ifdef _WIN32
        std::string currFolderName = cwdStr.substr(cwdStr.find_last_of("\\") + 1);
        if (currFolderName.compare("bin") != 0)
        {
            cwdStr = cwdStr.substr(0, cwdStr.find_last_of("\\"));  //  This will back up one directory to HOME/gui/lib
            cwdStr = cwdStr.substr(0, cwdStr.find_last_of("\\"));  //  This will back up one directory to HOME/gui
        }
        cwdStr = cwdStr.substr(0, cwdStr.find_last_of("\\"));  //  This will back up one directory to HOME

        if (SNT_SYNCH::Config::instance().FileExists(cwdStr + "\\bin\\qualnet.exe"))
        {
            productName = "QualNet";
        }
        else if (SNT_SYNCH::Config::instance().FileExists(cwdStr + "\\bin\\exata.exe"))
        {
            productName = "EXata";
        }
        else if (SNT_SYNCH::Config::instance().FileExists(cwdStr + "\\bin\\jne.exe"))
        {
            productName = "JNE";
        }
#else
        std::string currFolderName = cwdStr.substr(cwdStr.find_last_of("/") + 1);
        if (currFolderName.compare("bin") != 0)
        {
            cwdStr = cwdStr.substr(0, cwdStr.find_last_of("/"));  //  This will back up one directory to HOME/gui/lib
            cwdStr = cwdStr.substr(0, cwdStr.find_last_of("/"));  //  This will back up one directory to HOME/gui
        }
        cwdStr = cwdStr.substr(0, cwdStr.find_last_of("/"));  //  This will back up one directory to HOME

        if (SNT_SYNCH::Config::instance().FileExists(cwdStr + "/bin/qualnet"))
        {
            productName = "QualNet";
        }
        else if (SNT_SYNCH::Config::instance().FileExists(cwdStr + "/bin/exata"))
        {
            productName = "EXata";
        }
        else if (SNT_SYNCH::Config::instance().FileExists(cwdStr + "/bin/jne"))
        {
            productName = "JNE";
        }
#endif
        extractorHome = cwdStr.c_str();
    }
    else
    {
        extractorHome = ".";
        productName = "QualNet";
    }

    if (settings.value("showWelcome").isNull())
    {
        showWelcome = true;
    }
    else
    {
        showWelcome = settings.value("showWelcome").toBool();
    }

    if (showWelcome)
    {
        welcome = new Welcome(this);
        welcome->setWindowIcon(windowIcon);
        if (productName == "EXata")
        {
            welcome->label_4->setText(QString("The Extractor creates an %1 "
                                          "scenario from the identified "
                                          "HLA Federation or DIS Exercise.").
                                  arg(productName));
        }
        else
        {
            welcome->label_4->setText(QString("The Extractor creates a %1 "
                                          "scenario from the identified "
                                          "HLA Federation or DIS Exercise.").
                                  arg(productName));
        }
    }
    setup = new Setup(this);
    setup->setWindowIcon(windowIcon);
    if (!settings.value("defaultTimeout").isNull())
    {
        setup->timeout->setValue(settings.value("defaultTimeout").toInt());
    }

    adjustSubnets = new AdjustSubnets(this);
    adjustSubnets->setWindowIcon(windowIcon);
    adjustNames = new AdjustNames(this);
    adjustNames->setWindowIcon(windowIcon);
    updateFedAmb = new UpdateFedAmb(this);
    updateFedAmb->setWindowIcon(windowIcon);

    if (showWelcome)
    {
        connect(welcome->Cancel, SIGNAL(clicked()), this, SLOT(cleanUp()));
        connect(setup->Back, SIGNAL(clicked()), setup, SLOT(back()));

    }
    else
    {
        setup->Back->hide();
    }
    connect(setup->Cancel, SIGNAL(clicked()), this, SLOT(cleanUp()));
    connect(updateFedAmb->Cancel, SIGNAL(clicked()), this, SLOT(cleanUp()));
    connect(adjustSubnets->Cancel, SIGNAL(clicked()), this, SLOT(cleanUp()));
    connect(adjustNames->Cancel, SIGNAL(clicked()), this, SLOT(cleanUp()));
    connect(updateFedAmb, SIGNAL(canceled()), this, SLOT(cleanUp()));
}

void Extractor::Startup()
{
    QPixmap pixmap(":/splash.png");
    QSplashScreen splash(pixmap);
    splash.show();

    Config::instance().readParameterFiles();
    Config::instance().readModelFiles();
    setup->init();

    if (showWelcome)
    {
        welcome->show();
        splash.finish(welcome);
    }
    else
    {
        setup->show();
        splash.finish(setup);
    }

}

void Extractor::Extract()
{
    setup->hide();
    updateFedAmb->show();
    updateFedAmb->progressBar->setMaximum(0);
    updateFedAmb->progressBar->setValue(0);

    if (setup->useHla13->isChecked())
    {
        updateFedAmb->label_2->setVisible(true);
        updateFedAmb->updatesReceived->setVisible(true);
        Config::instance().externalInterfaceType = "HLA13";
        Config::instance().scenarioName =
            setup->scenarioName->text().toAscii().data();
        Config::instance().federationName =
            setup->federationName->text().toAscii().data();
        Config::instance().fedPath =
            setup->fedFileName->text().toAscii().data();
        Config::instance().rprVersion =
                setup->rprFomVersion->text().toAscii().data();
    }
    else if (setup->useHla1516->isChecked())
    {
        updateFedAmb->label_2->setVisible(true);
        updateFedAmb->updatesReceived->setVisible(true);
        Config::instance().externalInterfaceType = "HLA1516";
        Config::instance().scenarioName =
            setup->scenarioName->text().toAscii().data();
        Config::instance().federationName =
            setup->federationName->text().toAscii().data();
        Config::instance().fedPath =
            setup->fedFileName->text().toAscii().data();
        Config::instance().rprVersion =
                setup->rprFomVersion->text().toAscii().data();
    }
    else if (setup->useDis->isChecked())
    {
        updateFedAmb->label_2->setVisible(false);
        updateFedAmb->updatesReceived->setVisible(false);
        Config::instance().externalInterfaceType = "DIS";
        Config::instance().scenarioName =
            setup->scenarioName->text().toAscii().data();
        Config::instance().disDestIpAddress =
            setup->destinationAddress->text().toAscii().data();
        Config::instance().disDeviceAddress =
            setup->deviceAddress->text().toAscii().data();
        if (setup->subnetMaskCB->isChecked())
        {
            Config::instance().disSubnetMask =
                setup->subnetMask->text().toAscii().data();
        }
        Config::instance().disPort =
            setup->enteringPort->text().toAscii().data();
    }

    Config::instance().initializeScenarioParameters();

    if (ns)
    {
        delete ns;
    }
    ns = new SNT_SYNCH::NodeSet;

    if (!connectThread)
    {
        connectThread = new RtiConnectThread(this);
        connect(connectThread, SIGNAL(status(int, const QString &)), updateFedAmb, SLOT(updateStatus(int, const QString &)));
        connect(connectThread, SIGNAL(finished()), this, SLOT(connected()));
    }
    connectThread->start();
}

void Extractor::connected()
{
    if (connectThread && connectThread->connectShutdown())
        return;
    if (connectThread && connectThread->connectCompletedWithNoErrors())
        updateFedAmb->startCollecting(setup->timeout->value());
    else
    {
        if (!setup->useDis->isChecked())
        {
            QMessageBox::warning(NULL, "Error",
                QString("Failed to connect to the RTI. "
                "Please check the HLA Federation Settings."));
        }
        updateFedAmb->hide();
        setup->show();
    }
}

void RtiConnectThread::run()
{
    shutdownRequested = false;
    noErrors = false;

    VRLinkExternalInterface* vrlink = NULL;

    VRLinkInterfaceType type = GetVRLinkInterfaceType(
                                    Config::instance().externalInterfaceType);

    if (type == VRLINK_TYPE_DIS)
    {
        std::cout << "Initializing DIS connection\n\tDIS Dest IP Address "
            << Config::instance().disDestIpAddress
            << "\n\tDIS Network Address " << Config::instance().disDeviceAddress
            << "\n\tDIS port " << Config::instance().disPort
            << std::flush;
        vrlink = VRLinkInit(Config::instance().debug,
                      type,
                      (char*)Config::instance().scenarioName.c_str(),
                      (char*)Config::instance().disPort.c_str(),
                      (char*)Config::instance().disDestIpAddress.c_str(),
                      (char*)Config::instance().disDeviceAddress.c_str(),
                      (char*)Config::instance().disSubnetMask.c_str());
    }
    else if (type == VRLINK_TYPE_HLA13
             || type == VRLINK_TYPE_HLA1516)
    {
        emit status(1, "Connecting to RTI");
        std::cout << "Initializing Federation\n\tfederation name "
            << Config::instance().federationName
            << "\n\tFED file " << Config::instance().fedPath
            << "\n\tfederate name " << Config::instance().federateName
            << std::flush;
        vrlink = VRLinkInit(Config::instance().debug,
                      type,
                      (char*)Config::instance().scenarioName.c_str(),
                      (char*)Config::instance().rprVersion.c_str(),
                      (char*)Config::instance().federationName.c_str(),
                      (char*)Config::instance().federateName.c_str(),
                      (char*)Config::instance().fedPath.c_str());
    }

    extractor->vrlink = vrlink;

    if (shutdownRequested) {
        return;
    }

    std::cout << "Starting extraction process" << std::flush;
    emit status(2, "Subscribing to Entities");

    if (shutdownRequested) {
        return;
    }

    emit status(3, "Subscribing to Entities");

    if (shutdownRequested) {
        return;
    }

    noErrors = true;
}

int Extractor::WriteFiles()
{
    std::cout << "writing config files" << std::flush;
    QDir scenDir;
#ifdef _WIN32
    scenDir.mkpath(setup->scenarioDirectory->text() + "\\" + setup->scenarioName->text());
    QDir::setCurrent(setup->scenarioDirectory->text() + "\\" + setup->scenarioName->text());
#else
    scenDir.mkpath(setup->scenarioDirectory->text() + "/" +setup->scenarioName->text());
    QDir::setCurrent(setup->scenarioDirectory->text() + "/" + setup->scenarioName->text());
#endif
    Config::instance().scenarioDir = QDir::currentPath().toAscii().data();

    ConfigFileWriter cfWriter;

    if (Config::instance().externalInterfaceType == "HLA13"
        || Config::instance().externalInterfaceType == "HLA1516")
    {
        if (setup->copyFedFile->isChecked())
        {
            cfWriter.copyFederationFile();
            QFileInfo fedFileInfo(setup->fedFileName->text());
            Config::instance().fedPath = fedFileInfo.fileName().toAscii().data();
            Config::instance().setParameter(
                Parameter("VRLINK-FED-FILE-PATH", Config::instance().fedPath));
        }
    }

    cfWriter.writeRadiosFile(
        Config::instance().getParameter("VRLINK-RADIOS-FILE-PATH").value, *ns);
    cfWriter.writeEntitiesFile(
        Config::instance().getParameter("VRLINK-ENTITIES-FILE-PATH").value, *ns );
    cfWriter.writeNetworksFile(
        Config::instance().getParameter("VRLINK-NETWORKS-FILE-PATH").value, *ns);

    cfWriter.writeNodesFile(
        Config::instance().getParameter("NODE-POSITION-FILE").value, *ns);
    cfWriter.writeRouterModelFile(
        Config::instance().getParameter("ROUTER-MODEL-CONFIG-FILE").value, *ns);
    cfWriter.writeConfigFile(
        Config::instance().getParameter("CONFIG-FILE-PATH").value, *ns );
    cfWriter.copyIconFiles();

    return 0;
}

void Extractor::cleanUp()
{
    bool exitApplication = true;
    settings.sync();
    if (connectThread)
    {
        if (connectThread->isRunning())
        {
            connectThread->terminate();
            connectThread->wait();
        }
        delete connectThread;
        connectThread = 0;
    }
    if (welcome)
    {
        welcome->hide();
//        delete welcome;
//        welcome = 0;
    }
    if (adjustSubnets)
    {
        adjustSubnets->hide();
        adjustSubnets->timeToDie();
//        delete adjustSubnets;
//        adjustSubnets = 0;
    }
    if (adjustNames)
    {
        adjustNames->hide();
        adjustNames->timeToDie();
//        delete adjustNames;
//        adjustNames = 0;
    }
    if (updateFedAmb)
    {
        updateFedAmb->Stop();
        updateFedAmb->hide();

//        delete updateFedAmb;
//        updateFedAmb = 0;
    }
    if (setup)
    {
        setup->hide();
//        delete setup;
//        setup = 0;
    }
    if (dbW)
    {
        if (dbW->isVisible())
        {
            QMessageBox::StandardButton answer = QMessageBox::question(dbW, "Extractor", "The Extractor is closing. Do you wish to close the diagnostic output window?", QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
            if (answer == QMessageBox::Yes)
            {
                dbW->hide();
                delete dbW;
                dbW = 0;
            }
            else
            {
                exitApplication = false;
            }
        }
    }
    if (exitApplication)
        app->exit();
}
UpdateFedAmb::UpdateFedAmb(Extractor *ext) : QDialog(ext->setup, Qt::WindowTitleHint | Qt::WindowSystemMenuHint), extractor(ext)
{
    setupUi(this);
    timer = new QTimer(this);
    setWindowTitle("Extractor Step 2 - Extracting");
    status->setText("Starting Extraction");
    setWindowModality(Qt::WindowModal);
    progressBar->setMaximum(0);
    connect(Next, SIGNAL(clicked()), this, SLOT(accept()));
    connect(Back, SIGNAL(clicked()), this, SLOT(back()));
}
UpdateFedAmb::~UpdateFedAmb()
{
    if (timer)
        delete timer;
}
void UpdateFedAmb::setStatusLabels()
{
    int num;
    extractor->vrlink->Tick((clocktype)startTime.elapsed());
    extractor->vrlink->Receive();
    extractor->ns->extractNodes(extractor->vrlink);

    num = extractor->ns->size();
    entitiesDiscovered->setText(QString("%1").arg(num));

    num = extractor->ns->getEntitySet().getEntityUpdateCount();
    entitiesUpdated->setText(QString("%1").arg(num));

    num = extractor->ns->getRadioAddCount();
    radiosDiscovered->setText(QString("%1").arg(num));

    num = extractor->ns->getRadioUpdateCount();
    radiosUpdated->setText(QString("%1").arg(num));

    num = extractor->ns->getEntitySet().getEntityUpdateCount()
          + extractor->ns->getRadioUpdateCount();
    updatesReceived->setText(QString("%1").arg(num));

}
void UpdateFedAmb::startCollecting(int timeout)
{
    progressBar->setMaximum(1000*timeout);
    startTime.start();
    status->setText("Waiting for Entitiy Updates");
    setStatusLabels();
    done = false;
    connect(timer, SIGNAL(timeout()), this, SLOT(Collect()));
    timer->start(100);
}
void UpdateFedAmb::Collect()
{
    if (!done)
    {
        QSize sz = size();
        progressBar->setValue(startTime.elapsed());
        setStatusLabels();
        if (progressBar->maximum() > 0 && startTime.elapsed() > progressBar->maximum())
        {
            accept();
        }
    }
}
void UpdateFedAmb::Stop()
{
    done = true;
    timer->stop();
}
void UpdateFedAmb::closeEvent(QCloseEvent *evt)
{
    cancel();
}
void showWarningMessage( QString msg )
{
    QMessageBox msgBox(QMessageBox::Warning, "Warning",
        "Scenario could not be created.",
        QMessageBox::Ok);
    msgBox.setInformativeText(msg);
    QFont fnt = msgBox.font();
    fnt.setPointSize(10);
    msgBox.setFont(fnt);
    msgBox.exec();
}
void UpdateFedAmb::accept()
{
    Stop();
    if (extractor->connectThread
        && extractor->connectThread->isRunning())
    {
        extractor->connectThread->terminate();
        extractor->connectThread->wait();
        delete extractor->connectThread;
        extractor->connectThread = 0;
        if (!extractor->setup->useDis->isChecked())
        {
            showWarningMessage(tr(
                "<p>The Extractor could not connect to the RTI.</p>"
                "<p>You may need to increase the Extractor timeout or "
                "check your RTI configuration.</p>"
                ));
        }
        hide();
        extractor->setup->show();
        return;
    }

    status->setText("Creating scenario");
    extractor->ns->processExtractedNodes();

    if (extractor->ns->size() == 0)
    {
        hide();
        extractor->setup->show();
        return;
    }

    hide();
    extractor->adjustNames->show();
}
void UpdateFedAmb::back()
{
    extractor->setup->show();
    extractor->connectThread->requestShutdown();
    Stop();
    hide();
}
void UpdateFedAmb::cancel()
{
    Stop();
    hide();
    extractor->cleanUp();
}
void UpdateFedAmb::updateStatus(int step, const QString &message)
{
    progressBar->setValue(step);
    status->setText(message);
}

int main(int argc, char *argv[])
{

    QCoreApplication::setOrganizationName("ScalableNetworkTechnologies");
    QCoreApplication::setOrganizationDomain("scalable-networks.com");
    QCoreApplication::setApplicationName(EXTRACTOR_VERSION);

    QApplication app(argc, argv);
    Config::instance().parseCommandLine(argc, (const char**)argv);
    Extractor extractor(&app);
    app.setWindowIcon(extractor.windowIcon);
    extractor.Startup();
    return app.exec();
}
