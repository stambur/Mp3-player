// -*- c++ -*-
/*
 * Copyright (C) 2002 Zsolt Rizsanyi <rizsanyi@myrealbox.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */


#ifndef __LIRC_H__
#define __LIRC_H__

#include <QMap>
#include <QObject>
#include <QString>
//#include <config.h>
#include <lirc/lirc_client.h>

/**
 * Object for lirc support.
 */
class Lirc : public QObject
{
    Q_OBJECT

public:
    /**
     * Creates a lirc connection, and initializes configuration with appname @p appname.
     */
    Lirc( QObject *parent);
    virtual ~Lirc();

    /**
     * Sets the default keymap to use if there is no lirc config file, or default is specified
     * as command.
     * @p keymap will be deleted in the destructor
     */

signals:
    /**
     * an event received from lirc
     * you get this signal only if a not recognized key is pressed
     * @p key the name of the key pressed
     */
    void key_event(const QString& key);

    /**
     * lirc command received
     * @p cmd the cmd corresponding to pressed lirc key (as defined in the lirc config file)
     */
    //void command(const QString& cmd, unsigned repeat);

private slots:
    void dataReceived();

private:
    char* _appname;
    lirc_config* _config;
    int _lircfd;
};

#endif // __LIRC_H__
