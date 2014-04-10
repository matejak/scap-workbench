/*
 * Copyright 2014 Red Hat Inc., Durham, North Carolina.
 * All Rights Reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *      Martin Preisler <mpreisle@redhat.com>
 */

#include "RemoteMachineComboBox.h"
#include "OscapScannerRemoteSsh.h"

RemoteMachineComboBox::RemoteMachineComboBox(QWidget* parent):
    QWidget(parent)
{
    mUI.setupUi(this);

#if (QT_VERSION >= QT_VERSION_CHECK(4, 7, 0))
    // placeholder text is only supported in Qt 4.7 onwards
    mUI.host->setPlaceholderText("username@hostname");
#endif

    mQSettings = new QSettings(this);

    mRecentMenu = new QMenu(this);
    QObject::connect(
        mRecentMenu, SIGNAL(triggered(QAction*)),
        this, SLOT(recentMenuActionTriggered(QAction*))
    );
    mUI.recent->setMenu(mRecentMenu);

    setRecentMachineCount(5);
    syncFromQSettings();

}

RemoteMachineComboBox::~RemoteMachineComboBox()
{
    delete mRecentMenu;
    delete mQSettings;
}

QString RemoteMachineComboBox::getTarget() const
{
    return QString("%1:%2").arg(mUI.host->text()).arg(mUI.port->value());
}

void RemoteMachineComboBox::setRecentMachineCount(unsigned int count)
{
    while (mRecentTargets.size() > count)
        mRecentTargets.removeLast();

    while (mRecentTargets.size() < count)
        mRecentTargets.append("");
}

unsigned int RemoteMachineComboBox::getRecentMachineCount() const
{
    return mRecentTargets.size();
}

void RemoteMachineComboBox::notifyTargetUsed(const QString& target)
{
    const unsigned int machineCount = getRecentMachineCount();

    // this moves target to the beginning of the list of it was in the list already
    mRecentTargets.prepend(target);
    mRecentTargets.removeDuplicates();

    setRecentMachineCount(machineCount);

    syncToQSettings();
    syncRecentMenu();
}

void RemoteMachineComboBox::clearHistory()
{
    mUI.host->setText("");
    mUI.port->setValue(22);

    const unsigned int machineCount = getRecentMachineCount();
    mRecentTargets.clear();
    setRecentMachineCount(machineCount);

    syncToQSettings();
    syncRecentMenu();
}

void RemoteMachineComboBox::recentMenuActionTriggered(QAction* action)
{
    const QString& target = action->data().toString();
    if (target.isEmpty())
        return;

    QString host;
    short port;

    OscapScannerRemoteSsh::splitTarget(target, host, port);

    mUI.host->setText(host);
    mUI.port->setValue(port);
}

void RemoteMachineComboBox::syncFromQSettings()
{
    QVariant value = mQSettings->value("recent-remote-machines");
    QStringList list = value.toStringList();

    const unsigned int machineCount = getRecentMachineCount();
    mRecentTargets = list;
    setRecentMachineCount(machineCount);
    syncRecentMenu();
}

void RemoteMachineComboBox::syncToQSettings()
{
    mQSettings->setValue("recent-remote-machines", QVariant(mRecentTargets));
}

void RemoteMachineComboBox::syncRecentMenu()
{
    mRecentMenu->clear();

    bool empty = true;
    for (QStringList::iterator it = mRecentTargets.begin(); it != mRecentTargets.end(); ++it)
    {
        if (it->isEmpty())
            continue;

        QAction* action = new QAction(*it, mRecentMenu);
        action->setData(QVariant(*it));
        mRecentMenu->addAction(action);

        empty = false;
    }

    if (!empty)
    {
        mRecentMenu->addSeparator();
        QAction* clearHistory = new QAction("Clear History", mRecentMenu);
        QObject::connect(
            clearHistory, SIGNAL(triggered()),
            this, SLOT(clearHistory())
        );
        mRecentMenu->addAction(clearHistory);
    }

    mUI.recent->setEnabled(!empty);
}