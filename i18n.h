/*
 * i18n.h: Internationalization
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: i18n.h,v 1.2 2005/01/01 22:11:13 Exp $
 *
 */

#ifndef _I18N__H
#define _I18N__H

#include <vdr/i18n.h>
#include <vdr/config.h>         // for VDRVERSNUM define only

#if VDRVERSNUM < 10507
extern const tI18nPhrase tlPhrases[];
#endif

#endif //_I18N__H
