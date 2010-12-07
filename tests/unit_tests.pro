###############################################################################
#
# This file is part of libcommhistory.
#
# Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
# Contact: Alexander Shalamov <alexander.shalamov@nokia.com>
#
# This library is free software; you can redistribute it and/or modify it
# under the terms of the GNU Lesser General Public License version 2.1 as
# published by the Free Software Foundation.
#
# This library is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
# License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this library; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
#
###############################################################################

!include( ../common-vars.pri ):error( "Unable to install common-vars.pri" )

TEMPLATE = subdirs
SUBDIRS = model_example \
          group_example \
          async_example \
          ut_eventmodel \
          ut_callmodel \
          ut_conversationmodel \
          ut_draftmodel \
          ut_groupmodel \
          ut_outboxmodel \
          ut_smsinboxmodel \
          ut_syncmodel \
          ut_unreadeventsmodel \
          ut_classzerosmsmodel \
          ut_singleeventmodel
CONFIG += ordered

# make sure the destination path exists
!system( mkdir -p $${OUT_PWD}/bin ) : \
    error( "Unable to create bin dir for tests." )

#-----------------------------------------------------------------------------
# generate test xml
#-----------------------------------------------------------------------------
!system( ./do_tests_xml.sh $${OUT_PWD}/bin \
                    $${PROJECT_NAME}-tests \
                     \"$${SUBDIRS}\" ) : \
     error("Error running do_tests_xml.sh")
QMAKE_CLEAN += $${OUT_PWD}/bin/tests.xml

#-----------------------------------------------------------------------------
# installation setup
#-----------------------------------------------------------------------------
!include( ../common-installs-config.pri ) : \
         error( "Unable to include common-installs-config.pri!" )
autotests.files = $${OUT_PWD}/bin/*
autotests.path  = $${INSTALL_PREFIX}/share/$${PROJECT_NAME}-tests
INSTALLS += autotests