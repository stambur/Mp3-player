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

#include "lirc.h"
//#include "lirc.moc"
#include <qsocketnotifier.h>
#include <QDebug>
#include <fcntl.h>
#include <stdlib.h>

Lirc::Lirc( QObject *parent)
     :QObject( parent)
{
    _appname = "lirc";
    _config = 0;
    if ((_lircfd = lirc_init((char*)_appname, 1)) < 0) {
        qDebug() << "lirc: Failed to initialize" << endl;
        _lircfd = -1;
        return;
    }
    fcntl(_lircfd,F_SETFL,O_NONBLOCK);
    fcntl(_lircfd,F_SETFD,FD_CLOEXEC);

    if (lirc_readconfig(NULL, &_config, NULL) != 0) {
        qDebug() << "lirc: Couldn't read config file" << endl;
        _config = 0;
    }
    qDebug() << "lirc: Succesfully initialized" << endl;

    QSocketNotifier* sn = new QSocketNotifier( _lircfd, QSocketNotifier::Read, parent );
    QObject::connect( sn, SIGNAL(activated(int)), this, SLOT(dataReceived()) );
}

Lirc::~Lirc()
{
	if (_config)
	    lirc_freeconfig(_config);
	lirc_deinit();
}

void Lirc::dataReceived()
{
    if (_lircfd < 0)
	return;

    char *code, event[24];
    unsigned dummy, repeat;

    strcpy(event,"");
    while (lirc_nextcode(&code)==0  &&  code!=NULL) {
        if (3 != sscanf(code,"%x %x %20s",&dummy,&repeat,event)) {
            qDebug() << "lirc: oops, parse error: " << code << endl;
            free(code);
            continue;
        }
        qDebug() << "lirc: key '" << event << "' repeat " << repeat << endl;
        emit Lirc::key_event(event);

        free(code);
    }
}
