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
#include "Setup.h"
#include "FileNameWidget.h"
#include "DebugWindow.h"
#include "Welcome.h"

#include "Config.h"
#include <QMessageBox>
#include <sstream>

using namespace SNT_SYNCH;

#define RPR_FOM_VERSION_1 1.0
#define RPR_FOM_VERSION_2 2.0017

QString Setup::findFile(QString filename)
{
    QList<QFileInfo> fedFileInfo;
    fedFileInfo.append(QFileInfo(filename));
    fedFileInfo.append(QFileInfo(extractor->extractorHome + "/scenarios/vrf/data/" + fedFileInfo[0].fileName()));
    int i;
    for (i=0; i<fedFileInfo.size(); i++)
    {
        if (fedFileInfo[i].exists())
        {
            QString path = fedFileInfo[i].canonicalFilePath();
#ifdef _WIN32
            path = path.replace('/', '\\');
#endif
            return path;
        }
    }
    return QString("");
}

Setup::Setup(Extractor *ext) : QDialog(0, Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
                extractor(ext)
{
    setupUi(this);
    setSizeGripEnabled(true);

    for (int i = 0; i < tabWidget->count(); i++)
    {
        tabWidget->setTabEnabled(i, true);
    }
    tabWidget->setCurrentIndex(0);
    stackedWidget->setCurrentIndex(0);
    useHla13->setChecked(true);
    label_2->setVisible(true);
    copyFedFile->setVisible(true);

    QValidator* rprValidator = new QDoubleValidator();
    rprFomVersion->setValidator(rprValidator);
    rprFomVersion->setText("1.0");

    QValidator* portValidator = new QIntValidator(0, 65535, this);
    enteringPort->setValidator(portValidator);
    enteringPort->setText("3000");
    destinationAddress->setText("127.255.255.255");
    deviceAddress->setText("127.0.0.1");
    subnetMaskCB->setChecked(false);
    subnetMask->setText("");
    subnetMask->setVisible(false);

#ifdef _WIN32
    if (!extractor->extractorHome.isEmpty())
    {
        scenarioDirectory->setText(
            extractor->extractorHome + "\\scenarios\\user");
    }
    else if (!extractor->userHome.isEmpty())
    {
        scenarioDirectory->setText(extractor->userHome + "\\");
    }
#else
    if (!extractor->extractorHome.isEmpty())
    {
        scenarioDirectory->setText(
            extractor->extractorHome + "/scenarios/user");
    }
    else if (!extractor->userHome.isEmpty())
    {
        scenarioDirectory->setText(extractor->userHome + "/");
    }
#endif

    QString fedPath = findFile(SNT_SYNCH::Config::instance().fedPath.c_str());
    if (fedPath.isEmpty())
    {
        fedFileName->setText("");
    }
    else
    {
        fedFileName->setText(fedPath);
    }
    groupBox_2->setTitle(extractor->productName + QString(" Scenario Setting"));

    timeout->setSuffix("  seconds");
    timeout->setMaximum(99999);
    timeout->setSpecialValueText(tr("None"));

    connect(Next, SIGNAL(clicked()), this, SLOT(accept()));
    connect(findFedFile, SIGNAL(clicked()), this, SLOT(findFed()));
    connect(findScenarioDirectory, SIGNAL(clicked()), this, SLOT(findScnDir()));
    connect(scenarioDirectory, SIGNAL(textEdited(const QString &)), this, SLOT(setScnDir(const QString &)));
    connect(timeout, SIGNAL(valueChanged(int)), this, SLOT(handleTimeoutUpdate(int)));
    connect(useDis, SIGNAL(clicked()), this, SLOT(protocolChanged()));
    connect(useHla13, SIGNAL(clicked()), this, SLOT(protocolChanged()));
    connect(useHla1516, SIGNAL(clicked()), this, SLOT(protocolChanged()));
    connect(subnetMaskCB, SIGNAL(stateChanged(int)), this, SLOT(slotSubnetMaskCBClicked(int)));
}
void Setup::handleTimeoutUpdate(int i)
{
    if (i == 1)
        timeout->setSuffix("  second");
    else
        timeout->setSuffix("  seconds");
}

void Setup::init()
{
}

void Setup::accept()
{
    extractor->settings.setValue("defaultTimeout", timeout->value());

    if (useDis->isChecked())
    {
        if (enteringPort->text().trimmed().isEmpty())
        {
            QMessageBox::warning(this, tr("Extractor"),
                tr("You must set Entering port"), QMessageBox::Ok);
            return;
        }

        if (destinationAddress->text().trimmed().isEmpty())
        {
            QMessageBox::warning(this, tr("Extractor"),
                tr("You must set Destination Address"), QMessageBox::Ok);
            return;
        }
        else
        {
            QStringList address = destinationAddress->text().split('.');
            if (address.count() != 4)
            {
                QMessageBox::warning(this, tr("Extractor"),
                    tr("You must set valid Destination Address"),
                    QMessageBox::Ok);
                return;
            }
            bool ok = true;
            for (int i = 0; i < address.count(); i++)
            {
                address.at(i).toInt(&ok);
                if (!ok)
                {
                    QMessageBox::warning(this, tr("Extractor"),
                        tr("You must set valid Destination Address"),
                        QMessageBox::Ok);
                    return;
                }
            }
        }

        if (deviceAddress->text().trimmed().isEmpty())
        {
            QMessageBox::warning(this, tr("Extractor"),
                tr("You must set Network Device Address"), QMessageBox::Ok);
            return;
        }
        else
        {
            QStringList address = deviceAddress->text().split('.');
            if (address.count() != 4)
            {
                QMessageBox::warning(this, tr("Extractor"),
                    tr("You must set valid Network Device Address"),
                    QMessageBox::Ok);
                return;
            }
            bool ok = true;
            for (int i = 0; i < address.count(); i++)
            {
                address.at(i).toInt(&ok);
                if (!ok)
                {
                    QMessageBox::warning(this, tr("Extractor"),
                        tr("You must set valid Network Device Address"),
                        QMessageBox::Ok);
                    return;
                }
            }
        }

        if (subnetMaskCB->isChecked()
            && !subnetMask->text().isEmpty())
        {
            QStringList address = subnetMask->text().split('.');
            if (address.count() != 4)
            {
                QMessageBox::warning(this, tr("Extractor"),
                    tr("You must set valid Subnet Mask"),
                    QMessageBox::Ok);
                return;
            }
            bool ok = true;
            for (int i = 0; i < address.count(); i++)
            {
                address.at(i).toInt(&ok);
                if (!ok)
                {
                    QMessageBox::warning(this, tr("Extractor"),
                        tr("You must set valid Subnet Mask"),
                        QMessageBox::Ok);
                    return;
                }
            }
        }
    }
    else
    {
        if (useHla13->isChecked()
            && (fedFileName->text().isEmpty()
                || fedFileName->text().endsWith(".fed") == false))
        {
            QMessageBox::warning(this, tr("Extractor"),
                tr("You must select a FED file"), QMessageBox::Ok);
            return;
        }

        if (useHla1516->isChecked()
            && (fedFileName->text().isEmpty()
                || fedFileName->text().endsWith(".xml") == false))
        {
            QMessageBox::warning(this, tr("Extractor"),
                tr("You must select a FDD file"), QMessageBox::Ok);
            return;
        }

        if (federationName->text().isEmpty())
        {
            QMessageBox::warning(this, tr("Extractor"),
                tr("You must set Federation Name"), QMessageBox::Ok);
            return;
        }

        if (rprFomVersion->text().isEmpty())
        {
            QMessageBox::warning(this, tr("Extractor"),
                tr("You must set RPR Fom Version"), QMessageBox::Ok);
            return;
        }

        if (!(rprFomVersion->text().toDouble() == RPR_FOM_VERSION_1
              || rprFomVersion->text().toDouble() == RPR_FOM_VERSION_2))
        {
            QMessageBox::warning(this, tr("Extractor"),
                tr("You must set valid RPR Fom Version"), QMessageBox::Ok);
            return;
        }
    }

    QFileInfo scenDir(scenarioDirectory->text());
    if (scenDir.exists() && !scenDir.isDir())
    {
        QMessageBox::warning(this, tr("Extractor"),
            tr("Won't be able to create the scenarios\n"
               "There is a file with the same name as the scenario directory"),
               QMessageBox::Ok);
        return;
    }

    if (debugOutput->isChecked() && !extractor->dbW)
    {
        extractor->dbW = new DebugWindow;
        extractor->dbW->setWindowTitle("Extractor - Diagnostic Output");
        extractor->dbW->grabStream(std::cout);
        extractor->dbW->grabStream(std::cerr);
        extractor->dbW->grabStream(std::clog);
        extractor->dbW->resize(700,500);
        extractor->dbW->show();
    }
    extractor->Extract();
}

void Setup::findFed()
{
    QString filename;
    if (useHla13->isChecked())
    {
        filename = QFileDialog::getOpenFileName(this, tr("Open FED file"),
            extractor->exeHome + "\\gui", tr("FED Files (*.fed);;All Files (*.*)"));
    }
    else
    {
        filename = QFileDialog::getOpenFileName(this, tr("Open FDD file"),
            extractor->exeHome + "\\gui", tr("FDD Files (*.xml);;All Files (*.*)"));
    }
    if (!filename.isEmpty())
    {
#ifdef _WIN32
        filename = filename.replace('/', '\\');
#endif
        fedFileName->setText(filename);
    }
}
void Setup::findScnDir()
{
    QString dir;
    if (!scenarioDirectory->text().isEmpty())
    {
        dir = QFileDialog::getExistingDirectory(this, tr("Open Directory"),
            scenarioDirectory->text(), QFileDialog::ShowDirsOnly);
    }
    else
    {
#ifdef _WIN32
        dir = QFileDialog::getExistingDirectory(this, tr("Open Directory"),
            extractor->extractorHome + "\\scenarios\\user", QFileDialog::ShowDirsOnly);
#else
        dir = QFileDialog::getExistingDirectory(this, tr("Open Directory"),
            extractor->extractorHome + "/scenarios/user", QFileDialog::ShowDirsOnly);
#endif
    }

    if (!dir.isEmpty())
    {
        scenarioDirectory->setText(dir);
    }
}
void Setup::setScnDir(const QString &dir)
{
    extractor->settings.setValue("scenarioDirectory", dir);
}

void Setup::back()
{
    hide();
    extractor->welcome->show();
}

void Setup::protocolChanged()
{
    if (useDis->isChecked())
    {
        label_2->setVisible(false);
        copyFedFile->setVisible(false);
        stackedWidget->setCurrentIndex(1);
    }
    else
    {
        label_2->setVisible(true);
        copyFedFile->setVisible(true);
        stackedWidget->setCurrentIndex(0);

        if (useHla13->isChecked())
        {
            label_3->setText("Federation Execution Data (FED) file:");
            label_3->setToolTip("Enter the path and file name of the FED file");
            SNT_SYNCH::Config::instance().fedPath = "VR-Link.fed";
        }
        else
        {
            label_3->setText("Federation Document Data (FDD) file:");
            label_3->setToolTip("Enter the path and file name of the FDD file");
            SNT_SYNCH::Config::instance().fedPath = "VR-Link.xml";
        }

        QString fedPath = findFile(SNT_SYNCH::Config::instance().fedPath.c_str());
        if (fedPath.isEmpty())
        {
            fedFileName->setText("");
        }
        else
        {
            fedFileName->setText(fedPath);
        }
    }
}

void Setup::slotSubnetMaskCBClicked(int state)
{
    subnetMask->setVisible(state);
}
