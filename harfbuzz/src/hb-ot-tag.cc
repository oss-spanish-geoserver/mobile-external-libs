/*
 * Copyright © 2009  Red Hat, Inc.
 * Copyright © 2011  Google, Inc.
 *
 *  This is part of HarfBuzz, a text shaping library.
 *
 * Permission is hereby granted, without written agreement and without
 * license or royalty fees, to use, copy, modify, and distribute this
 * software and its documentation for any purpose, provided that the
 * above copyright notice and the following two paragraphs appear in
 * all copies of this software.
 *
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
 * ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN
 * IF THE COPYRIGHT HOLDER HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 * THE COPYRIGHT HOLDER SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING,
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE COPYRIGHT HOLDER HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 *
 * Red Hat Author(s): Behdad Esfahbod
 * Google Author(s): Behdad Esfahbod, Roozbeh Pournader
 */

#include "hb-private.hh"

#include <string.h>



/* hb_script_t */

static hb_tag_t
hb_ot_old_tag_from_script (hb_script_t script)
{
  /* This seems to be accurate as of end of 2012. */

  switch ((hb_tag_t) script) {
    case HB_SCRIPT_INVALID:		return HB_OT_TAG_DEFAULT_SCRIPT;

    /* KATAKANA and HIRAGANA both map to 'kana' */
    case HB_SCRIPT_HIRAGANA:		return HB_TAG('k','a','n','a');

    /* Spaces at the end are preserved, unlike ISO 15924 */
    case HB_SCRIPT_LAO:			return HB_TAG('l','a','o',' ');
    case HB_SCRIPT_YI:			return HB_TAG('y','i',' ',' ');
    /* Unicode-5.0 additions */
    case HB_SCRIPT_NKO:			return HB_TAG('n','k','o',' ');
    /* Unicode-5.1 additions */
    case HB_SCRIPT_VAI:			return HB_TAG('v','a','i',' ');
    /* Unicode-5.2 additions */
    /* Unicode-6.0 additions */
  }

  /* Else, just change first char to lowercase and return */
  return ((hb_tag_t) script) | 0x20000000u;
}

static hb_script_t
hb_ot_old_tag_to_script (hb_tag_t tag)
{
  if (unlikely (tag == HB_OT_TAG_DEFAULT_SCRIPT))
    return HB_SCRIPT_INVALID;

  /* This side of the conversion is fully algorithmic. */

  /* Any spaces at the end of the tag are replaced by repeating the last
   * letter.  Eg 'nko ' -> 'Nkoo' */
  if (unlikely ((tag & 0x0000FF00u) == 0x00002000u))
    tag |= (tag >> 8) & 0x0000FF00u; /* Copy second letter to third */
  if (unlikely ((tag & 0x000000FFu) == 0x00000020u))
    tag |= (tag >> 8) & 0x000000FFu; /* Copy third letter to fourth */

  /* Change first char to uppercase and return */
  return (hb_script_t) (tag & ~0x20000000u);
}

static hb_tag_t
hb_ot_new_tag_from_script (hb_script_t script)
{
  switch ((hb_tag_t) script) {
    case HB_SCRIPT_BENGALI:		return HB_TAG('b','n','g','2');
    case HB_SCRIPT_DEVANAGARI:		return HB_TAG('d','e','v','2');
    case HB_SCRIPT_GUJARATI:		return HB_TAG('g','j','r','2');
    case HB_SCRIPT_GURMUKHI:		return HB_TAG('g','u','r','2');
    case HB_SCRIPT_KANNADA:		return HB_TAG('k','n','d','2');
    case HB_SCRIPT_MALAYALAM:		return HB_TAG('m','l','m','2');
    case HB_SCRIPT_ORIYA:		return HB_TAG('o','r','y','2');
    case HB_SCRIPT_TAMIL:		return HB_TAG('t','m','l','2');
    case HB_SCRIPT_TELUGU:		return HB_TAG('t','e','l','2');
    case HB_SCRIPT_MYANMAR:		return HB_TAG('m','y','m','2');
  }

  return HB_OT_TAG_DEFAULT_SCRIPT;
}

static hb_script_t
hb_ot_new_tag_to_script (hb_tag_t tag)
{
  switch (tag) {
    case HB_TAG('b','n','g','2'):	return HB_SCRIPT_BENGALI;
    case HB_TAG('d','e','v','2'):	return HB_SCRIPT_DEVANAGARI;
    case HB_TAG('g','j','r','2'):	return HB_SCRIPT_GUJARATI;
    case HB_TAG('g','u','r','2'):	return HB_SCRIPT_GURMUKHI;
    case HB_TAG('k','n','d','2'):	return HB_SCRIPT_KANNADA;
    case HB_TAG('m','l','m','2'):	return HB_SCRIPT_MALAYALAM;
    case HB_TAG('o','r','y','2'):	return HB_SCRIPT_ORIYA;
    case HB_TAG('t','m','l','2'):	return HB_SCRIPT_TAMIL;
    case HB_TAG('t','e','l','2'):	return HB_SCRIPT_TELUGU;
    case HB_TAG('m','y','m','2'):	return HB_SCRIPT_MYANMAR;
  }

  return HB_SCRIPT_UNKNOWN;
}

/*
 * Complete list at:
 * https://www.microsoft.com/typography/otspec/scripttags.htm
 * https://www.microsoft.com/typography/otspec160/scripttagsProposed.htm
 *
 * Most of the script tags are the same as the ISO 15924 tag but lowercased.
 * So we just do that, and handle the exceptional cases in a switch.
 */

void
hb_ot_tags_from_script (hb_script_t  script,
			hb_tag_t    *script_tag_1,
			hb_tag_t    *script_tag_2)
{
  hb_tag_t new_tag;

  *script_tag_2 = HB_OT_TAG_DEFAULT_SCRIPT;
  *script_tag_1 = hb_ot_old_tag_from_script (script);

  new_tag = hb_ot_new_tag_from_script (script);
  if (unlikely (new_tag != HB_OT_TAG_DEFAULT_SCRIPT)) {
    *script_tag_2 = *script_tag_1;
    *script_tag_1 = new_tag;
  }
}

hb_script_t
hb_ot_tag_to_script (hb_tag_t tag)
{
  if (unlikely ((tag & 0x000000FFu) == '2'))
    return hb_ot_new_tag_to_script (tag);

  return hb_ot_old_tag_to_script (tag);
}


/* hb_language_t */

typedef struct {
  char language[4];
  hb_tag_t tag;
} LangTag;

/*
 * Complete list at:
 * http://www.microsoft.com/typography/otspec/languagetags.htm
 *
 * Generated by intersecting the OpenType language tag list from
 * Draft OpenType 1.5 spec, with with the ISO 639-3 codes from
 * 2008-08-04, matching on name, and finally adjusted manually.
 *
 * Updated on 2012-12-07 with more research into remaining codes.
 *
 * Updated on 2013-11-23 based on usage in SIL and Microsoft fonts,
 * the new proposal from Microsoft, and latest ISO 639-3 names.
 *
 * Some items still missing.  Those are commented out at the end.
 * Keep sorted for bsearch.
 *
 * Updated as of 2015-05-06: OT1.7 on MS website has some newer
 * items that we don't have here, eg. Zazaki.  This is the new
 * items in OpenType 1.7 (red items), most of which we have:
 * http://www.microsoft.com/typography/otspec170/languagetags.htm
 */

static const LangTag ot_languages[] = {
  {"aa",	HB_TAG('A','F','R',' ')},	/* Afar */
  {"ab",	HB_TAG('A','B','K',' ')},	/* Abkhazian */
  {"abq",	HB_TAG('A','B','A',' ')},	/* Abaza */
  {"acf",	HB_TAG('F','A','N',' ')},	/* French Antillean */
  {"ach",	HB_TAG('A','C','H',' ')},	/* Acoli */
  {"acr",	HB_TAG('A','C','R',' ')},	/* Achi */
  {"ada",	HB_TAG('D','N','G',' ')},	/* Dangme */
  {"ady",	HB_TAG('A','D','Y',' ')},	/* Adyghe */
  {"af",	HB_TAG('A','F','K',' ')},	/* Afrikaans */
  {"ahg",	HB_TAG('A','G','W',' ')},	/* Agaw */
  {"aii",	HB_TAG('S','W','A',' ')},	/* Swadaya Aramaic */
  {"aio",	HB_TAG('A','I','O',' ')},	/* Aiton */
  {"aiw",	HB_TAG('A','R','I',' ')},	/* Aari */
  {"ak",	HB_TAG('T','W','I',' ')},	/* Akan [macrolanguage] */
  {"aka",	HB_TAG('A','K','A',' ')},	/* Akan */
  {"alt",	HB_TAG('A','L','T',' ')},	/* [Southern] Altai */
  {"am",	HB_TAG('A','M','H',' ')},	/* Amharic */
  {"amf",	HB_TAG('H','B','N',' ')},	/* Hammer-Banna */
  {"an",	HB_TAG('A','R','G',' ')},	/* Aragonese */
  {"ang",	HB_TAG('A','N','G',' ')},	/* Old English (ca. 450-1100) */
  {"ar",	HB_TAG('A','R','A',' ')},	/* Arabic [macrolanguage] */
  {"arb",	HB_TAG('A','R','A',' ')},	/* Standard Arabic */
  {"arn",	HB_TAG('M','A','P',' ')},	/* Mapudungun */
  {"ary",	HB_TAG('M','O','R',' ')},	/* Moroccan Arabic */
  {"as",	HB_TAG('A','S','M',' ')},	/* Assamese */
  {"ast",	HB_TAG('A','S','T',' ')},	/* Asturian/Asturleonese/Bable/Leonese */
  {"ath",	HB_TAG('A','T','H',' ')},	/* Athapaskan [family] */
  {"atj",	HB_TAG('R','C','R',' ')},	/* R-Cree */
  {"atv",	HB_TAG('A','L','T',' ')},	/* [Northern] Altai */
  {"av",	HB_TAG('A','V','R',' ')},	/* Avaric */
  {"awa",	HB_TAG('A','W','A',' ')},	/* Awadhi */
  {"ay",	HB_TAG('A','Y','M',' ')},	/* Aymara [macrolanguage] */
  {"az",	HB_TAG('A','Z','E',' ')},	/* Azerbaijani [macrolanguage] */
  {"azb",	HB_TAG('A','Z','B',' ')},	/* South Azerbaijani */
  {"azj",	HB_TAG('A','Z','E',' ')},	/* North Azerbaijani */
  {"ba",	HB_TAG('B','S','H',' ')},	/* Bashkir */
  {"bad",	HB_TAG('B','A','D','0')},	/* Banda */
  {"bai",	HB_TAG('B','M','L',' ')},	/* Bamileke [family] */
  {"bal",	HB_TAG('B','L','I',' ')},	/* Baluchi [macrolangauge] */
  {"ban",	HB_TAG('B','A','N',' ')},	/* Balinese */
  {"bar",	HB_TAG('B','A','R',' ')},	/* Bavarian */
  {"bbc",	HB_TAG('B','B','C',' ')},	/* Batak Toba */
  {"bci",	HB_TAG('B','A','U',' ')},	/* Baoulé */
  {"bcl",	HB_TAG('B','I','K',' ')},	/* Central Bikol */
  {"bcq",	HB_TAG('B','C','H',' ')},	/* Bench */
  {"bdy",	HB_TAG('B','D','Y',' ')},	/* Bandjalang */
  {"be",	HB_TAG('B','E','L',' ')},	/* Belarusian */
  {"bem",	HB_TAG('B','E','M',' ')},	/* Bemba (Zambia) */
  {"ber",	HB_TAG('B','E','R',' ')},	/* Berber [family] */
  {"bfq",	HB_TAG('B','A','D',' ')},	/* Badaga */
  {"bft",	HB_TAG('B','L','T',' ')},	/* Balti */
  {"bfu",	HB_TAG('L','A','H',' ')},	/* Lahuli */
  {"bfy",	HB_TAG('B','A','G',' ')},	/* Baghelkhandi */
  {"bg",	HB_TAG('B','G','R',' ')},	/* Bulgarian */
  {"bgc",	HB_TAG('B','G','C',' ')},	/* Haryanvi */
  {"bgq",	HB_TAG('B','G','Q',' ')},	/* Bagri */
  {"bhb",	HB_TAG('B','H','I',' ')},	/* Bhili */
  {"bhk",	HB_TAG('B','I','K',' ')},	/* Albay Bicolano (retired code) */
  {"bho",	HB_TAG('B','H','O',' ')},	/* Bhojpuri */
  {"bi",	HB_TAG('B','I','S',' ')},	/* Bislama */
  {"bik",	HB_TAG('B','I','K',' ')},	/* Bikol [macrolanguage] */
  {"bin",	HB_TAG('E','D','O',' ')},	/* Bini */
  {"bjj",	HB_TAG('B','J','J',' ')},	/* Kanauji */
  {"bjt",	HB_TAG('B','L','N',' ')},	/* Balanta-Ganja */
  {"bla",	HB_TAG('B','K','F',' ')},	/* Blackfoot */
  {"ble",	HB_TAG('B','L','N',' ')},	/* Balanta-Kentohe */
  {"blk",	HB_TAG('B','L','K',' ')},	/* Pa'O/Pa'o Karen */
  {"bln",	HB_TAG('B','I','K',' ')},	/* Southern Catanduanes Bikol */
  {"bm",	HB_TAG('B','M','B',' ')},	/* Bambara */
  {"bn",	HB_TAG('B','E','N',' ')},	/* Bengali */
  {"bo",	HB_TAG('T','I','B',' ')},	/* Tibetan */
  {"bpy",	HB_TAG('B','P','Y',' ')},	/* Bishnupriya */
  {"bqi",	HB_TAG('L','R','C',' ')},	/* Bakhtiari */
  {"br",	HB_TAG('B','R','E',' ')},	/* Breton */
  {"bra",	HB_TAG('B','R','I',' ')},	/* Braj Bhasha */
  {"brh",	HB_TAG('B','R','H',' ')},	/* Brahui */
  {"brx",	HB_TAG('B','R','X',' ')},	/* Bodo (India) */
  {"bs",	HB_TAG('B','O','S',' ')},	/* Bosnian */
  {"btb",	HB_TAG('B','T','I',' ')},	/* Beti (Cameroon) */
  {"bto",	HB_TAG('B','I','K',' ')},	/* Rinconada Bikol */
  {"bts",	HB_TAG('B','T','S',' ')},	/* Batak Simalungun */
  {"bug",	HB_TAG('B','U','G',' ')},	/* Buginese */
  {"bxr",	HB_TAG('R','B','U',' ')},	/* Russian Buriat */
  {"byn",	HB_TAG('B','I','L',' ')},	/* Bilen */
  {"ca",	HB_TAG('C','A','T',' ')},	/* Catalan */
  {"cak",	HB_TAG('C','A','K',' ')},	/* Kaqchikel */
  {"cbk",	HB_TAG('C','B','K',' ')},	/* Chavacano */
  {"ce",	HB_TAG('C','H','E',' ')},	/* Chechen */
  {"ceb",	HB_TAG('C','E','B',' ')},	/* Cebuano */
  {"cgg",	HB_TAG('C','G','G',' ')},	/* Chiga */
  {"ch",	HB_TAG('C','H','A',' ')},	/* Chamorro */
  {"chk",	HB_TAG('C','H','K','0')},	/* Chuukese */
  {"cho",	HB_TAG('C','H','O',' ')},	/* Choctaw */
  {"chp",	HB_TAG('C','H','P',' ')},	/* Chipewyan */
  {"chr",	HB_TAG('C','H','R',' ')},	/* Cherokee */
  {"chy",	HB_TAG('C','H','Y',' ')},	/* Cheyenne */
  {"ckb",	HB_TAG('K','U','R',' ')},	/* Central Kurdish (Sorani) */
  {"ckt",	HB_TAG('C','H','K',' ')},	/* Chukchi */
  {"cop",	HB_TAG('C','O','P',' ')},	/* Coptic */
  {"cpp",	HB_TAG('C','P','P',' ')},	/* Creoles */
  {"cr",	HB_TAG('C','R','E',' ')},	/* Cree */
  {"cre",	HB_TAG('Y','C','R',' ')},	/* Y-Cree */
  {"crh",	HB_TAG('C','R','T',' ')},	/* Crimean Tatar */
  {"crj",	HB_TAG('E','C','R',' ')},	/* [Southern] East Cree */
  {"crk",	HB_TAG('W','C','R',' ')},	/* West-Cree */
  {"crl",	HB_TAG('E','C','R',' ')},	/* [Northern] East Cree */
  {"crm",	HB_TAG('M','C','R',' ')},	/* Moose Cree */
  {"crx",	HB_TAG('C','R','R',' ')},	/* Carrier */
  {"cs",	HB_TAG('C','S','Y',' ')},	/* Czech */
  {"csb",	HB_TAG('C','S','B',' ')},	/* Kashubian */
  {"ctg",	HB_TAG('C','T','G',' ')},	/* Chittagonian */
  {"cts",	HB_TAG('B','I','K',' ')},	/* Northern Catanduanes Bikol */
  {"cu",	HB_TAG('C','S','L',' ')},	/* Church Slavic */
  {"cuk",	HB_TAG('C','U','K',' ')},	/* San Blas Kuna */
  {"cv",	HB_TAG('C','H','U',' ')},	/* Chuvash */
  {"cwd",	HB_TAG('D','C','R',' ')},	/* Woods Cree */
  {"cy",	HB_TAG('W','E','L',' ')},	/* Welsh */
  {"da",	HB_TAG('D','A','N',' ')},	/* Danish */
  {"dap",	HB_TAG('N','I','S',' ')},	/* Nisi (India) */
  {"dar",	HB_TAG('D','A','R',' ')},	/* Dargwa */
  {"dax",	HB_TAG('D','A','X',' ')},	/* Dayi */
  {"de",	HB_TAG('D','E','U',' ')},	/* German */
  {"dgo",	HB_TAG('D','G','O',' ')},	/* Dogri */
  {"dhd",	HB_TAG('M','A','W',' ')},	/* Dhundari */
  {"dhg",	HB_TAG('D','H','G',' ')},	/* Dhangu */
  {"din",	HB_TAG('D','N','K',' ')},	/* Dinka [macrolanguage] */
  {"diq",	HB_TAG('D','I','Q',' ')},	/* Dimli */
  {"dje",	HB_TAG('D','J','R',' ')},	/* Zarma */
  {"djr",	HB_TAG('D','J','R','0')},	/* Djambarrpuyngu */
  {"dng",	HB_TAG('D','U','N',' ')},	/* Dungan */
  {"dnj",	HB_TAG('D','N','J',' ')},	/* Dan */
  {"doi",	HB_TAG('D','G','R',' ')},	/* Dogri [macrolanguage] */
  {"dsb",	HB_TAG('L','S','B',' ')},	/* Lower Sorbian */
  {"duj",	HB_TAG('D','U','J',' ')},	/* Dhuwal */
  {"dv",	HB_TAG('D','I','V',' ')},	/* Dhivehi/Divehi/Maldivian */
  {"dyu",	HB_TAG('J','U','L',' ')},	/* Jula */
  {"dz",	HB_TAG('D','Z','N',' ')},	/* Dzongkha */
  {"ee",	HB_TAG('E','W','E',' ')},	/* Ewe */
  {"efi",	HB_TAG('E','F','I',' ')},	/* Efik */
  {"ekk",	HB_TAG('E','T','I',' ')},	/* Standard Estonian */
  {"el",	HB_TAG('E','L','L',' ')},	/* Modern Greek (1453-) */
  {"emk",	HB_TAG('M','N','K',' ')},	/* Eastern Maninkakan */
  {"en",	HB_TAG('E','N','G',' ')},	/* English */
  {"enf",	HB_TAG('F','N','E',' ')},	/* Forest Nenets */
  {"enh",	HB_TAG('T','N','E',' ')},	/* Tundra Nenets */
  {"eo",	HB_TAG('N','T','O',' ')},	/* Esperanto */
  {"eot",	HB_TAG('B','T','I',' ')},	/* Beti (Côte d'Ivoire) */
  {"es",	HB_TAG('E','S','P',' ')},	/* Spanish */
  {"esu",	HB_TAG('E','S','U',' ')},	/* Central Yupik */
  {"et",	HB_TAG('E','T','I',' ')},	/* Estonian [macrolanguage] */
  {"eu",	HB_TAG('E','U','Q',' ')},	/* Basque */
  {"eve",	HB_TAG('E','V','N',' ')},	/* Even */
  {"evn",	HB_TAG('E','V','K',' ')},	/* Evenki */
  {"fa",	HB_TAG('F','A','R',' ')},	/* Persian [macrolanguage] */
  {"fan",	HB_TAG('F','A','N','0')},	/* Fang */
  {"fat",	HB_TAG('F','A','T',' ')},	/* Fanti */
  {"ff",	HB_TAG('F','U','L',' ')},	/* Fulah [macrolanguage] */
  {"fi",	HB_TAG('F','I','N',' ')},	/* Finnish */
  {"fil",	HB_TAG('P','I','L',' ')},	/* Filipino */
  {"fj",	HB_TAG('F','J','I',' ')},	/* Fijian */
  {"flm",	HB_TAG('H','A','L',' ')},	/* Halam */
  {"fo",	HB_TAG('F','O','S',' ')},	/* Faroese */
  {"fon",	HB_TAG('F','O','N',' ')},	/* Fon */
  {"fr",	HB_TAG('F','R','A',' ')},	/* French */
  {"frc",	HB_TAG('F','R','C',' ')},	/* Cajun French */
  {"frp",	HB_TAG('F','R','P',' ')},	/* Arpitan/Francoprovençal */
  {"fuf",	HB_TAG('F','T','A',' ')},	/* Futa */
  {"fur",	HB_TAG('F','R','L',' ')},	/* Friulian */
  {"fuv",	HB_TAG('F','U','V',' ')},	/* Nigerian Fulfulde */
  {"fy",	HB_TAG('F','R','I',' ')},	/* Western Frisian */
  {"ga",	HB_TAG('I','R','I',' ')},	/* Irish */
  {"gaa",	HB_TAG('G','A','D',' ')},	/* Ga */
  {"gag",	HB_TAG('G','A','G',' ')},	/* Gagauz */
  {"gbm",	HB_TAG('G','A','W',' ')},	/* Garhwali */
  {"gd",	HB_TAG('G','A','E',' ')},	/* Scottish Gaelic */
  {"gez",	HB_TAG('G','E','Z',' ')},	/* Ge'ez */
  {"ggo",	HB_TAG('G','O','N',' ')},	/* Southern Gondi */
  {"gih",	HB_TAG('G','I','H',' ')},	/* Githabul */
  {"gil",	HB_TAG('G','I','L','0')},	/* Kiribati (Gilbertese) */
  {"gkp",	HB_TAG('G','K','P',' ')},	/* Kpelle (Guinea) */
  {"gl",	HB_TAG('G','A','L',' ')},	/* Galician */
  {"gld",	HB_TAG('N','A','N',' ')},	/* Nanai */
  {"gle",	HB_TAG('I','R','T',' ')},	/* Irish Traditional */
  {"glk",	HB_TAG('G','L','K',' ')},	/* Gilaki */
  {"gn",	HB_TAG('G','U','A',' ')},	/* Guarani [macrolanguage] */
  {"gnn",	HB_TAG('G','N','N',' ')},	/* Gumatj */
  {"gno",	HB_TAG('G','O','N',' ')},	/* Northern Gondi */
  {"gog",	HB_TAG('G','O','G',' ')},	/* Gogo */
  {"gon",	HB_TAG('G','O','N',' ')},	/* Gondi [macrolanguage] */
  {"grt",	HB_TAG('G','R','O',' ')},	/* Garo */
  {"gru",	HB_TAG('S','O','G',' ')},	/* Sodo Gurage */
  {"gsw",	HB_TAG('A','L','S',' ')},	/* Alsatian */
  {"gu",	HB_TAG('G','U','J',' ')},	/* Gujarati */
  {"guc",	HB_TAG('G','U','C',' ')},	/* Wayuu */
  {"guf",	HB_TAG('G','U','F',' ')},	/* Gupapuyngu */
  {"guk",	HB_TAG('G','M','Z',' ')},	/* Gumuz */
/*{"guk",	HB_TAG('G','U','K',' ')},*/	/* Gumuz (in SIL fonts) */
  {"guz",	HB_TAG('G','U','Z',' ')},	/* Ekegusii/Gusii */
  {"gv",	HB_TAG('M','N','X',' ')},	/* Manx */
  {"ha",	HB_TAG('H','A','U',' ')},	/* Hausa */
  {"har",	HB_TAG('H','R','I',' ')},	/* Harari */
  {"haw",	HB_TAG('H','A','W',' ')},	/* Hawaiian */
  {"hay",	HB_TAG('H','A','Y',' ')},	/* Haya */
  {"haz",	HB_TAG('H','A','Z',' ')},	/* Hazaragi */
  {"he",	HB_TAG('I','W','R',' ')},	/* Hebrew */
  {"hi",	HB_TAG('H','I','N',' ')},	/* Hindi */
  {"hil",	HB_TAG('H','I','L',' ')},	/* Hiligaynon */
  {"hmn",	HB_TAG('H','M','N',' ')},	/* Hmong */
  {"hnd",	HB_TAG('H','N','D',' ')},	/* [Southern] Hindko */
  {"hne",	HB_TAG('C','H','H',' ')},	/* Chattisgarhi */
  {"hno",	HB_TAG('H','N','D',' ')},	/* [Northern] Hindko */
  {"ho",	HB_TAG('H','M','O',' ')},	/* Hiri Motu */
  {"hoc",	HB_TAG('H','O',' ',' ')},	/* Ho */
  {"hoj",	HB_TAG('H','A','R',' ')},	/* Harauti */
  {"hr",	HB_TAG('H','R','V',' ')},	/* Croatian */
  {"hsb",	HB_TAG('U','S','B',' ')},	/* Upper Sorbian */
  {"ht",	HB_TAG('H','A','I',' ')},	/* Haitian/Haitian Creole */
  {"hu",	HB_TAG('H','U','N',' ')},	/* Hungarian */
  {"hy",	HB_TAG('H','Y','E',' ')},	/* Armenian */
  {"hz",	HB_TAG('H','E','R',' ')},	/* Herero */
  {"ia",	HB_TAG('I','N','A',' ')},	/* Interlingua (International Auxiliary Language Association) */
  {"iba",	HB_TAG('I','B','A',' ')},	/* Iban */
  {"ibb",	HB_TAG('I','B','B',' ')},	/* Ibibio */
  {"id",	HB_TAG('I','N','D',' ')},	/* Indonesian */
  {"ie",	HB_TAG('I','L','E',' ')},	/* Interlingue/Occidental */
  {"ig",	HB_TAG('I','B','O',' ')},	/* Igbo */
  {"igb",	HB_TAG('E','B','I',' ')},	/* Ebira */
  {"ii",	HB_TAG('Y','I','M',' ')},	/* Yi Modern */
  {"ijc",	HB_TAG('I','J','O',' ')},	/* Izon */
  {"ijo",	HB_TAG('I','J','O',' ')},	/* Ijo [family] */
  {"ik",	HB_TAG('I','P','K',' ')},	/* Inupiaq [macrolanguage] */
  {"ilo",	HB_TAG('I','L','O',' ')},	/* Ilokano */
  {"inh",	HB_TAG('I','N','G',' ')},	/* Ingush */
  {"io",	HB_TAG('I','D','O',' ')},	/* Ido */
  {"is",	HB_TAG('I','S','L',' ')},	/* Icelandic */
  {"it",	HB_TAG('I','T','A',' ')},	/* Italian */
  {"iu",	HB_TAG('I','N','U',' ')},	/* Inuktitut [macrolanguage] */
  {"ja",	HB_TAG('J','A','N',' ')},	/* Japanese */
  {"jam",	HB_TAG('J','A','M',' ')},	/* Jamaican Creole English */
  {"jbo",	HB_TAG('J','B','O',' ')},	/* Lojban */
  {"jv",	HB_TAG('J','A','V',' ')},	/* Javanese */
  {"ka",	HB_TAG('K','A','T',' ')},	/* Georgian */
  {"kaa",	HB_TAG('K','R','K',' ')},	/* Karakalpak */
  {"kab",	HB_TAG('K','A','B','0')},	/* Kabyle */
  {"kam",	HB_TAG('K','M','B',' ')},	/* Kamba (Kenya) */
  {"kar",	HB_TAG('K','R','N',' ')},	/* Karen [family] */
  {"kat",	HB_TAG('K','G','E',' ')},	/* Khutsuri Georgian */
  {"kbd",	HB_TAG('K','A','B',' ')},	/* Kabardian */
  {"kde",	HB_TAG('K','D','E',' ')},	/* Makonde */
  {"kdr",	HB_TAG('K','R','M',' ')},	/* Karaim */
  {"kdt",	HB_TAG('K','U','Y',' ')},	/* Kuy */
  {"kea",	HB_TAG('K','E','A',' ')},	/* Kabuverdianu (Crioulo) */
  {"kek",	HB_TAG('K','E','K',' ')},	/* Kekchi */
  {"kex",	HB_TAG('K','K','N',' ')},	/* Kokni */
  {"kfa",	HB_TAG('K','O','D',' ')},	/* Kodagu */
  {"kfr",	HB_TAG('K','A','C',' ')},	/* Kachchi */
  {"kfx",	HB_TAG('K','U','L',' ')},	/* Kulvi */
  {"kfy",	HB_TAG('K','M','N',' ')},	/* Kumaoni */
  {"kg",	HB_TAG('K','O','N',' ')},	/* Kongo [macrolanguage] */
  {"kha",	HB_TAG('K','S','I',' ')},	/* Khasi */
  {"khb",	HB_TAG('X','B','D',' ')},	/* Lü */
  {"kht",	HB_TAG('K','H','N',' ')},	/* Khamti (Microsoft fonts) */
/*{"kht",	HB_TAG('K','H','T',' ')},*/	/* Khamti (OpenType spec and SIL fonts) */
  {"khw",	HB_TAG('K','H','W',' ')},	/* Khowar */
  {"ki",	HB_TAG('K','I','K',' ')},	/* Gikuyu/Kikuyu */
  {"kiu",	HB_TAG('K','I','U',' ')},	/* Kirmanjki */
  {"kj",	HB_TAG('K','U','A',' ')},	/* Kuanyama/Kwanyama */
  {"kjd",	HB_TAG('K','J','D',' ')},	/* Southern Kiwai */
  {"kjh",	HB_TAG('K','H','A',' ')},	/* Khakass */
  {"kjp",	HB_TAG('K','J','P',' ')},	/* Pwo Eastern Karen */
  {"kk",	HB_TAG('K','A','Z',' ')},	/* Kazakh */
  {"kl",	HB_TAG('G','R','N',' ')},	/* Kalaallisut */
  {"kln",	HB_TAG('K','A','L',' ')},	/* Kalenjin */
  {"km",	HB_TAG('K','H','M',' ')},	/* Central Khmer */
  {"kmb",	HB_TAG('M','B','N',' ')},	/* Kimbundu */
  {"kmw",	HB_TAG('K','M','O',' ')},	/* Komo (Democratic Republic of Congo) */
  {"kn",	HB_TAG('K','A','N',' ')},	/* Kannada */
  {"knn",	HB_TAG('K','O','K',' ')},	/* Konkani */
  {"ko",	HB_TAG('K','O','R',' ')},	/* Korean */
  {"koi",	HB_TAG('K','O','P',' ')},	/* Komi-Permyak */
  {"kok",	HB_TAG('K','O','K',' ')},	/* Konkani [macrolanguage] */
  {"kon",	HB_TAG('K','O','N','0')},	/* Kongo */
  {"kos",	HB_TAG('K','O','S',' ')},	/* Kosraean */
  {"kpe",	HB_TAG('K','P','L',' ')},	/* Kpelle [macrolanguage] */
  {"kpv",	HB_TAG('K','O','Z',' ')},	/* Komi-Zyrian */
  {"kpy",	HB_TAG('K','Y','K',' ')},	/* Koryak */
  {"kqy",	HB_TAG('K','R','T',' ')},	/* Koorete */
  {"kr",	HB_TAG('K','N','R',' ')},	/* Kanuri [macrolanguage] */
  {"kri",	HB_TAG('K','R','I',' ')},	/* Krio */
  {"krl",	HB_TAG('K','R','L',' ')},	/* Karelian */
  {"kru",	HB_TAG('K','U','U',' ')},	/* Kurukh */
  {"ks",	HB_TAG('K','S','H',' ')},	/* Kashmiri */
  {"ksh",	HB_TAG('K','S','H','0')},	/* Ripuarian, Kölsch */
/*{"ksw",	HB_TAG('K','R','N',' ')},*/	/* S'gaw Karen (Microsoft fonts?) */
  {"ksw",	HB_TAG('K','S','W',' ')},	/* S'gaw Karen (OpenType spec and SIL fonts) */
  {"ktb",	HB_TAG('K','E','B',' ')},	/* Kebena */
  {"ktu",	HB_TAG('K','O','N',' ')},	/* Kikongo */
  {"ku",	HB_TAG('K','U','R',' ')},	/* Kurdish [macrolanguage] */
  {"kum",	HB_TAG('K','U','M',' ')},	/* Kumyk */
  {"kv",	HB_TAG('K','O','M',' ')},	/* Komi [macrolanguage] */
  {"kvd",	HB_TAG('K','U','I',' ')},	/* Kui (Indonesia) */
  {"kw",	HB_TAG('C','O','R',' ')},	/* Cornish */
  {"kxc",	HB_TAG('K','M','S',' ')},	/* Komso */
  {"kxu",	HB_TAG('K','U','I',' ')},	/* Kui (India) */
  {"ky",	HB_TAG('K','I','R',' ')},	/* Kirghiz/Kyrgyz */
  {"kyu",	HB_TAG('K','Y','U',' ')},	/* Western Kayah */
  {"la",	HB_TAG('L','A','T',' ')},	/* Latin */
  {"lad",	HB_TAG('J','U','D',' ')},	/* Ladino */
  {"lb",	HB_TAG('L','T','Z',' ')},	/* Luxembourgish */
  {"lbe",	HB_TAG('L','A','K',' ')},	/* Lak */
  {"lbj",	HB_TAG('L','D','K',' ')},	/* Ladakhi */
  {"lez",	HB_TAG('L','E','Z',' ')},	/* Lezgi */
  {"lg",	HB_TAG('L','U','G',' ')},	/* Ganda */
  {"li",	HB_TAG('L','I','M',' ')},	/* Limburgan/Limburger/Limburgish */
  {"lif",	HB_TAG('L','M','B',' ')},	/* Limbu */
  {"lij",	HB_TAG('L','I','J',' ')},	/* Ligurian */
  {"lis",	HB_TAG('L','I','S',' ')},	/* Lisu */
  {"ljp",	HB_TAG('L','J','P',' ')},	/* Lampung Api */
  {"lki",	HB_TAG('L','K','I',' ')},	/* Laki */
  {"lld",	HB_TAG('L','A','D',' ')},	/* Ladin */
  {"lmn",	HB_TAG('L','A','M',' ')},	/* Lambani */
  {"lmo",	HB_TAG('L','M','O',' ')},	/* Lombard */
  {"ln",	HB_TAG('L','I','N',' ')},	/* Lingala */
  {"lo",	HB_TAG('L','A','O',' ')},	/* Lao */
  {"lom",	HB_TAG('L','O','M',' ')},	/* Loma */
  {"lrc",	HB_TAG('L','R','C',' ')},	/* Northern Luri */
  {"lt",	HB_TAG('L','T','H',' ')},	/* Lithuanian */
  {"lu",	HB_TAG('L','U','B',' ')},	/* Luba-Katanga */
  {"lua",	HB_TAG('L','U','B',' ')},	/* Luba-Kasai */
  {"luo",	HB_TAG('L','U','O',' ')},	/* Luo (Kenya and Tanzania) */
  {"lus",	HB_TAG('M','I','Z',' ')},	/* Mizo */
  {"luy",	HB_TAG('L','U','H',' ')},	/* Luyia/Oluluyia [macrolanguage] */
  {"luz",	HB_TAG('L','R','C',' ')},	/* Southern Luri */
  {"lv",	HB_TAG('L','V','I',' ')},	/* Latvian */
  {"lzz",	HB_TAG('L','A','Z',' ')},	/* Laz */
  {"mad",	HB_TAG('M','A','D',' ')},	/* Madurese */
  {"mag",	HB_TAG('M','A','G',' ')},	/* Magahi */
  {"mai",	HB_TAG('M','T','H',' ')},	/* Maithili */
  {"mak",	HB_TAG('M','K','R',' ')},	/* Makasar */
  {"mal",	HB_TAG('M','A','L',' ')},	/* Malayalam */
  {"mam",	HB_TAG('M','A','M',' ')},	/* Mam */
  {"man",	HB_TAG('M','N','K',' ')},	/* Manding/Mandingo [macrolanguage] */
  {"mdc",	HB_TAG('M','L','E',' ')},	/* Male (Papua New Guinea) */
  {"mdf",	HB_TAG('M','O','K',' ')},	/* Moksha */
  {"mdr",	HB_TAG('M','D','R',' ')},	/* Mandar */
  {"mdy",	HB_TAG('M','L','E',' ')},	/* Male (Ethiopia) */
  {"men",	HB_TAG('M','D','E',' ')},	/* Mende (Sierra Leone) */
  {"mer",	HB_TAG('M','E','R',' ')},	/* Meru */
  {"mfe",	HB_TAG('M','F','E',' ')},	/* Morisyen */
  {"mg",	HB_TAG('M','L','G',' ')},	/* Malagasy [macrolanguage] */
  {"mh",	HB_TAG('M','A','H',' ')},	/* Marshallese */
  {"mhr",	HB_TAG('L','M','A',' ')},	/* Low Mari */
  {"mi",	HB_TAG('M','R','I',' ')},	/* Maori */
  {"min",	HB_TAG('M','I','N',' ')},	/* Minangkabau */
  {"mk",	HB_TAG('M','K','D',' ')},	/* Macedonian */
  {"mku",	HB_TAG('M','N','K',' ')},	/* Konyanka Maninka */
  {"mkw",	HB_TAG('M','K','W',' ')},	/* Kituba (Congo) */
  {"ml",	HB_TAG('M','L','R',' ')},	/* Malayalam */
  {"mlq",	HB_TAG('M','N','K',' ')},	/* Western Maninkakan */
  {"mn",	HB_TAG('M','N','G',' ')},	/* Mongolian [macrolanguage] */
  {"mnc",	HB_TAG('M','C','H',' ')},	/* Manchu */
  {"mni",	HB_TAG('M','N','I',' ')},	/* Manipuri */
  {"mnk",	HB_TAG('M','N','D',' ')},	/* Mandinka */
  {"mns",	HB_TAG('M','A','N',' ')},	/* Mansi */
  {"mnw",	HB_TAG('M','O','N',' ')},	/* Mon */
  {"mo",	HB_TAG('M','O','L',' ')},	/* Moldavian */
  {"moh",	HB_TAG('M','O','H',' ')},	/* Mohawk */
  {"mos",	HB_TAG('M','O','S',' ')},	/* Mossi */
  {"mpe",	HB_TAG('M','A','J',' ')},	/* Majang */
  {"mr",	HB_TAG('M','A','R',' ')},	/* Marathi */
  {"mrj",	HB_TAG('H','M','A',' ')},	/* High Mari */
  {"ms",	HB_TAG('M','L','Y',' ')},	/* Malay [macrolanguage] */
  {"msc",	HB_TAG('M','N','K',' ')},	/* Sankaran Maninka */
  {"mt",	HB_TAG('M','T','S',' ')},	/* Maltese */
  {"mtr",	HB_TAG('M','A','W',' ')},	/* Mewari */
  {"mus",	HB_TAG('M','U','S',' ')},	/* Creek */
  {"mve",	HB_TAG('M','A','W',' ')},	/* Marwari (Pakistan) */
  {"mwk",	HB_TAG('M','N','K',' ')},	/* Kita Maninkakan */
  {"mwl",	HB_TAG('M','W','L',' ')},	/* Mirandese */
  {"mwr",	HB_TAG('M','A','W',' ')},	/* Marwari [macrolanguage] */
  {"mww",	HB_TAG('M','W','W',' ')},	/* Hmong Daw */
  {"my",	HB_TAG('B','R','M',' ')},	/* Burmese */
  {"mym",	HB_TAG('M','E','N',' ')},	/* Me'en */
  {"myn",	HB_TAG('M','Y','N',' ')},	/* Mayan */
  {"myq",	HB_TAG('M','N','K',' ')},	/* Forest Maninka (retired code) */
  {"myv",	HB_TAG('E','R','Z',' ')},	/* Erzya */
  {"mzn",	HB_TAG('M','Z','N',' ')},	/* Mazanderani */
  {"na",	HB_TAG('N','A','U',' ')},	/* Nauru */
  {"nag",	HB_TAG('N','A','G',' ')},	/* Naga-Assamese */
  {"nah",	HB_TAG('N','A','H',' ')},	/* Nahuatl [family] */
  {"nap",	HB_TAG('N','A','P',' ')},	/* Neapolitan */
  {"nb",	HB_TAG('N','O','R',' ')},	/* Norwegian Bokmål */
  {"nco",	HB_TAG('S','I','B',' ')},	/* Sibe */
  {"nd",	HB_TAG('N','D','B',' ')},	/* [North] Ndebele */
  {"ndc",	HB_TAG('N','D','C',' ')},	/* Ndau */
  {"nds",	HB_TAG('N','D','S',' ')},	/* Low German/Low Saxon */
  {"ne",	HB_TAG('N','E','P',' ')},	/* Nepali */
  {"new",	HB_TAG('N','E','W',' ')},	/* Newari */
  {"ng",	HB_TAG('N','D','G',' ')},	/* Ndonga */
  {"nga",	HB_TAG('N','G','A',' ')},	/* Ngabaka */
  {"ngl",	HB_TAG('L','M','W',' ')},	/* Lomwe */
  {"ngo",	HB_TAG('S','X','T',' ')},	/* Sutu */
  {"niu",	HB_TAG('N','I','U',' ')},	/* Niuean */
  {"niv",	HB_TAG('G','I','L',' ')},	/* Gilyak */
  {"nl",	HB_TAG('N','L','D',' ')},	/* Dutch */
  {"nn",	HB_TAG('N','Y','N',' ')},	/* Norwegian Nynorsk */
  {"no",	HB_TAG('N','O','R',' ')},	/* Norwegian [macrolanguage] */
  {"nod",	HB_TAG('N','T','A',' ')},	/* Northern Thai */
  {"noe",	HB_TAG('N','O','E',' ')},	/* Nimadi */
  {"nog",	HB_TAG('N','O','G',' ')},	/* Nogai */
  {"nov",	HB_TAG('N','O','V',' ')},	/* Novial */
  {"nqo",	HB_TAG('N','K','O',' ')},	/* N'Ko */
  {"nr",	HB_TAG('N','D','B',' ')},	/* [South] Ndebele */
  {"nsk",	HB_TAG('N','A','S',' ')},	/* Naskapi */
  {"nso",	HB_TAG('S','O','T',' ')},	/* [Northern] Sotho */
  {"nv",	HB_TAG('N','A','V',' ')},	/* Navajo */
  {"ny",	HB_TAG('C','H','I',' ')},	/* Chewa/Chichwa/Nyanja */
  {"nym",	HB_TAG('N','Y','M',' ')},	/* Nyamwezi */
  {"nyn",	HB_TAG('N','K','L',' ')},	/* Nyankole */
  {"oc",	HB_TAG('O','C','I',' ')},	/* Occitan (post 1500) */
  {"oj",	HB_TAG('O','J','B',' ')},	/* Ojibwa [macrolanguage] */
  {"ojs",	HB_TAG('O','C','R',' ')},	/* Oji-Cree */
  {"okm",	HB_TAG('K','O','H',' ')},	/* Korean Old Hangul */
  {"om",	HB_TAG('O','R','O',' ')},	/* Oromo [macrolanguage] */
  {"or",	HB_TAG('O','R','I',' ')},	/* Oriya */
  {"os",	HB_TAG('O','S','S',' ')},	/* Ossetian */
  {"pa",	HB_TAG('P','A','N',' ')},	/* Panjabi */
  {"pag",	HB_TAG('P','A','G',' ')},	/* Pangasinan */
  {"pam",	HB_TAG('P','A','M',' ')},	/* Kapampangan/Pampanga */
  {"pap",	HB_TAG('P','A','P','0')},	/* Papiamento */
  {"pau",	HB_TAG('P','A','U',' ')},	/* Palauan */
  {"pcc",	HB_TAG('P','C','C',' ')},	/* Bouyei */
  {"pcd",	HB_TAG('P','C','D',' ')},	/* Picard */
  {"pce",	HB_TAG('P','L','G',' ')},	/* [Ruching] Palaung */
  {"pdc",	HB_TAG('P','D','C',' ')},	/* Pennsylvania German */
  {"pes",	HB_TAG('F','A','R',' ')},	/* Iranian Persian */
  {"phk",	HB_TAG('P','H','K',' ')},	/* Phake */
  {"pi",	HB_TAG('P','A','L',' ')},	/* Pali */
  {"pih",	HB_TAG('P','I','H',' ')},	/* Pitcairn-Norfolk */
  {"pl",	HB_TAG('P','L','K',' ')},	/* Polish */
  {"pll",	HB_TAG('P','L','G',' ')},	/* [Shwe] Palaung */
  {"plp",	HB_TAG('P','A','P',' ')},	/* Palpa */
  {"pms",	HB_TAG('P','M','S',' ')},	/* Piemontese */
  {"pnb",	HB_TAG('P','N','B',' ')},	/* Western Panjabi */
  {"poh",	HB_TAG('P','O','H',' ')},	/* Pocomchi */
  {"pon",	HB_TAG('P','O','N',' ')},	/* Pohnpeian */
  {"prs",	HB_TAG('D','R','I',' ')},	/* Afghan Persian/Dari */
  {"ps",	HB_TAG('P','A','S',' ')},	/* Pashto/Pushto [macrolanguage] */
  {"pt",	HB_TAG('P','T','G',' ')},	/* Portuguese */
  {"pwo",	HB_TAG('P','W','O',' ')},	/* Pwo Western Karen */
  {"qu",	HB_TAG('Q','U','Z',' ')},	/* Quechua [macrolanguage] */
  {"quc",	HB_TAG('Q','U','C',' ')},	/* K'iche'/Quiché */
  {"quh",	HB_TAG('Q','U','H',' ')},	/* Quechua (Bolivia) */
  {"quz",	HB_TAG('Q','U','Z',' ')},	/* Cusco Quechua */
  {"qvi",	HB_TAG('Q','V','I',' ')},	/* Quechua (Ecuador) */
  {"qwh",	HB_TAG('Q','W','H',' ')},	/* Quechua (Peru) */
  {"raj",	HB_TAG('R','A','J',' ')},	/* Rajasthani [macrolanguage] */
  {"rar",	HB_TAG('R','A','R',' ')},	/* Rarotongan */
  {"rbb",	HB_TAG('P','L','G',' ')},	/* Rumai Palaung */
  {"rej",	HB_TAG('R','E','J',' ')},	/* Rejang */
  {"ria",	HB_TAG('R','I','A',' ')},	/* Riang (India) */
  {"rif",	HB_TAG('R','I','F',' ')},	/* Tarifit */
  {"ril",	HB_TAG('R','I','A',' ')},	/* Riang (Myanmar) */
  {"rit",	HB_TAG('R','I','T',' ')},	/* Ritarungo */
  {"rki",	HB_TAG('A','R','K',' ')},	/* Rakhine */
  {"rkw",	HB_TAG('R','K','W',' ')},	/* Arakwal */
  {"rm",	HB_TAG('R','M','S',' ')},	/* Romansh */
  {"rmy",	HB_TAG('R','M','Y',' ')},	/* Vlax Romani */
  {"rn",	HB_TAG('R','U','N',' ')},	/* Rundi */
  {"ro",	HB_TAG('R','O','M',' ')},	/* Romanian */
  {"rom",	HB_TAG('R','O','Y',' ')},	/* Romany [macrolanguage] */
  {"rtm",	HB_TAG('R','T','M',' ')},	/* Rotuman */
  {"ru",	HB_TAG('R','U','S',' ')},	/* Russian */
  {"rue",	HB_TAG('R','S','Y',' ')},	/* Rusyn */
  {"rup",	HB_TAG('R','U','P',' ')},	/* Aromanian/Arumanian/Macedo-Romanian */
  {"rw",	HB_TAG('R','U','A',' ')},	/* Kinyarwanda */
  {"rwr",	HB_TAG('M','A','W',' ')},	/* Marwari (India) */
  {"sa",	HB_TAG('S','A','N',' ')},	/* Sanskrit */
  {"sah",	HB_TAG('Y','A','K',' ')},	/* Yakut */
  {"sam",	HB_TAG('P','A','A',' ')},	/* Palestinian Aramaic */
  {"sas",	HB_TAG('S','A','S',' ')},	/* Sasak */
  {"sat",	HB_TAG('S','A','T',' ')},	/* Santali */
  {"sc",	HB_TAG('S','R','D',' ')},	/* Sardinian [macrolanguage] */
  {"sck",	HB_TAG('S','A','D',' ')},	/* Sadri */
  {"scn",	HB_TAG('S','C','N',' ')},	/* Sicilian */
  {"sco",	HB_TAG('S','C','O',' ')},	/* Scots */
  {"scs",	HB_TAG('S','L','A',' ')},	/* [North] Slavey */
  {"sd",	HB_TAG('S','N','D',' ')},	/* Sindhi */
  {"se",	HB_TAG('N','S','M',' ')},	/* Northern Sami */
  {"seh",	HB_TAG('S','N','A',' ')},	/* Sena */
  {"sel",	HB_TAG('S','E','L',' ')},	/* Selkup */
  {"sg",	HB_TAG('S','G','O',' ')},	/* Sango */
  {"sga",	HB_TAG('S','G','A',' ')},	/* Old Irish (to 900) */
  {"sgs",	HB_TAG('S','G','S',' ')},	/* Samogitian */
  {"sgw",	HB_TAG('C','H','G',' ')},	/* Sebat Bet Gurage */
/*{"sgw",	HB_TAG('S','G','W',' ')},*/	/* Sebat Bet Gurage (in SIL fonts) */
  {"shi",	HB_TAG('S','H','I',' ')},	/* Tachelhit */
  {"shn",	HB_TAG('S','H','N',' ')},	/* Shan */
  {"si",	HB_TAG('S','N','H',' ')},	/* Sinhala */
  {"sid",	HB_TAG('S','I','D',' ')},	/* Sidamo */
  {"sjd",	HB_TAG('K','S','M',' ')},	/* Kildin Sami */
  {"sk",	HB_TAG('S','K','Y',' ')},	/* Slovak */
  {"skr",	HB_TAG('S','R','K',' ')},	/* Seraiki */
  {"sl",	HB_TAG('S','L','V',' ')},	/* Slovenian */
  {"sm",	HB_TAG('S','M','O',' ')},	/* Samoan */
  {"sma",	HB_TAG('S','S','M',' ')},	/* Southern Sami */
  {"smj",	HB_TAG('L','S','M',' ')},	/* Lule Sami */
  {"smn",	HB_TAG('I','S','M',' ')},	/* Inari Sami */
  {"sms",	HB_TAG('S','K','S',' ')},	/* Skolt Sami */
  {"sn",	HB_TAG('S','N','A','0')},	/* Shona */
  {"snk",	HB_TAG('S','N','K',' ')},	/* Soninke */
  {"so",	HB_TAG('S','M','L',' ')},	/* Somali */
  {"sop",	HB_TAG('S','O','P',' ')},	/* Songe */
  {"sq",	HB_TAG('S','Q','I',' ')},	/* Albanian [macrolanguage] */
  {"sr",	HB_TAG('S','R','B',' ')},	/* Serbian */
  {"srr",	HB_TAG('S','R','R',' ')},	/* Serer */
  {"ss",	HB_TAG('S','W','Z',' ')},	/* Swati */
  {"st",	HB_TAG('S','O','T',' ')},	/* [Southern] Sotho */
  {"stq",	HB_TAG('S','T','Q',' ')},	/* Saterfriesisch */
  {"stv",	HB_TAG('S','I','G',' ')},	/* Silt'e */
  {"su",	HB_TAG('S','U','N',' ')},	/* Sundanese */
  {"suk",	HB_TAG('S','U','K',' ')},	/* Sukama */
  {"suq",	HB_TAG('S','U','R',' ')},	/* Suri */
  {"sv",	HB_TAG('S','V','E',' ')},	/* Swedish */
  {"sva",	HB_TAG('S','V','A',' ')},	/* Svan */
  {"sw",	HB_TAG('S','W','K',' ')},	/* Swahili [macrolanguage] */
  {"swb",	HB_TAG('C','M','R',' ')},	/* Comorian */
  {"swh",	HB_TAG('S','W','K',' ')},	/* Kiswahili/Swahili */
  {"swv",	HB_TAG('M','A','W',' ')},	/* Shekhawati */
  {"sxu",	HB_TAG('S','X','U',' ')},	/* Upper Saxon */
  {"syl",	HB_TAG('S','Y','L',' ')},	/* Sylheti */
  {"syr",	HB_TAG('S','Y','R',' ')},	/* Syriac [macrolanguage] */
  {"szl",	HB_TAG('S','Z','L',' ')},	/* Silesian */
  {"ta",	HB_TAG('T','A','M',' ')},	/* Tamil */
  {"tab",	HB_TAG('T','A','B',' ')},	/* Tabasaran */
  {"tcy",	HB_TAG('T','U','L',' ')},	/* Tulu */
  {"tdd",	HB_TAG('T','D','D',' ')},	/* Tai Nüa */
  {"te",	HB_TAG('T','E','L',' ')},	/* Telugu */
  {"tem",	HB_TAG('T','M','N',' ')},	/* Temne */
  {"tet",	HB_TAG('T','E','T',' ')},	/* Tetum */
  {"tg",	HB_TAG('T','A','J',' ')},	/* Tajik */
  {"th",	HB_TAG('T','H','A',' ')},	/* Thai */
  {"ti",	HB_TAG('T','G','Y',' ')},	/* Tigrinya */
  {"tig",	HB_TAG('T','G','R',' ')},	/* Tigre */
  {"tiv",	HB_TAG('T','I','V',' ')},	/* Tiv */
  {"tk",	HB_TAG('T','K','M',' ')},	/* Turkmen */
  {"tl",	HB_TAG('T','G','L',' ')},	/* Tagalog */
  {"tmh",	HB_TAG('T','M','H',' ')},	/* Tamashek */
  {"tn",	HB_TAG('T','N','A',' ')},	/* Tswana */
  {"to",	HB_TAG('T','G','N',' ')},	/* Tonga (Tonga Islands) */
  {"tod",	HB_TAG('T','O','D','0')},	/* Toma */
  {"toi",	HB_TAG('T','N','G',' ')},	/* Tonga */
  {"tpi",	HB_TAG('T','P','I',' ')},	/* Tok Pisin */
  {"tr",	HB_TAG('T','R','K',' ')},	/* Turkish */
  {"tru",	HB_TAG('T','U','A',' ')},	/* Turoyo Aramaic */
  {"ts",	HB_TAG('T','S','G',' ')},	/* Tsonga */
  {"tt",	HB_TAG('T','A','T',' ')},	/* Tatar */
  {"tum",	HB_TAG('T','U','M',' ')},	/* Tumbuka */
  {"tvl",	HB_TAG('T','V','L',' ')},	/* Tuvalu */
  {"tw",	HB_TAG('T','W','I',' ')},	/* Twi */
  {"ty",	HB_TAG('T','H','T',' ')},	/* Tahitian */
  {"tyv",	HB_TAG('T','U','V',' ')},	/* Tuvin */
  {"tyz",	HB_TAG('T','Y','Z',' ')},	/* Tày */
  {"tzm",	HB_TAG('T','Z','M',' ')},	/* Central Atlas Tamazight */
  {"tzo",	HB_TAG('T','Z','O',' ')},	/* Tzotzil */
  {"udm",	HB_TAG('U','D','M',' ')},	/* Udmurt */
  {"ug",	HB_TAG('U','Y','G',' ')},	/* Uighur */
  {"uk",	HB_TAG('U','K','R',' ')},	/* Ukrainian */
  {"umb",	HB_TAG('U','M','B',' ')},	/* Umbundu */
  {"unr",	HB_TAG('M','U','N',' ')},	/* Mundari */
  {"ur",	HB_TAG('U','R','D',' ')},	/* Urdu */
  {"uz",	HB_TAG('U','Z','B',' ')},	/* Uzbek [macrolanguage] */
  {"uzn",	HB_TAG('U','Z','B',' ')},	/* Northern Uzbek */
  {"uzs",	HB_TAG('U','Z','B',' ')},	/* Southern Uzbek */
  {"ve",	HB_TAG('V','E','N',' ')},	/* Venda */
  {"vec",	HB_TAG('V','E','C',' ')},	/* Venetian */
  {"vi",	HB_TAG('V','I','T',' ')},	/* Vietnamese */
  {"vls",	HB_TAG('F','L','E',' ')},	/* Vlaams */
  {"vmw",	HB_TAG('M','A','K',' ')},	/* Makhuwa */
  {"vo",	HB_TAG('V','O','L',' ')},	/* Volapük */
  {"vro",	HB_TAG('V','R','O',' ')},	/* Võro */
  {"wa",	HB_TAG('W','L','N',' ')},	/* Walloon */
  {"war",	HB_TAG('W','A','R',' ')},	/* Waray (Philippines) */
  {"wbm",	HB_TAG('W','A',' ',' ')},	/* Wa */
  {"wbr",	HB_TAG('W','A','G',' ')},	/* Wagdi */
  {"wle",	HB_TAG('S','I','G',' ')},	/* Wolane */
  {"wo",	HB_TAG('W','L','F',' ')},	/* Wolof */
  {"wry",	HB_TAG('M','A','W',' ')},	/* Merwari */
  {"wtm",	HB_TAG('W','T','M',' ')},	/* Mewati */
  {"xal",	HB_TAG('K','L','M',' ')},	/* Kalmyk */
  {"xan",	HB_TAG('S','E','K',' ')},	/* Sekota */
  {"xh",	HB_TAG('X','H','S',' ')},	/* Xhosa */
  {"xjb",	HB_TAG('X','J','B',' ')},	/* Minjangbal */
  {"xog",	HB_TAG('X','O','G',' ')},	/* Soga */
  {"xom",	HB_TAG('K','M','O',' ')},	/* Komo (Sudan) */
  {"xpe",	HB_TAG('X','P','E',' ')},	/* Kpelle (Liberia) */
  {"xsl",	HB_TAG('S','S','L',' ')},	/* South Slavey */
  {"xst",	HB_TAG('S','I','G',' ')},	/* Silt'e (retired code) */
  {"xwo",	HB_TAG('T','O','D',' ')},	/* Written Oirat (Todo) */
  {"yao",	HB_TAG('Y','A','O',' ')},	/* Yao */
  {"yap",	HB_TAG('Y','A','P',' ')},	/* Yapese */
  {"yi",	HB_TAG('J','I','I',' ')},	/* Yiddish [macrolanguage] */
  {"yo",	HB_TAG('Y','B','A',' ')},	/* Yoruba */
  {"yso",	HB_TAG('N','I','S',' ')},	/* Nisi (China) */
  {"za",	HB_TAG('Z','H','A',' ')},	/* Chuang/Zhuang [macrolanguage] */
  {"zea",	HB_TAG('Z','E','A',' ')},	/* Zeeuws */
  {"zgh",	HB_TAG('Z','G','H',' ')},	/* Standard Morrocan Tamazigh */
  {"zne",	HB_TAG('Z','N','D',' ')},	/* Zande */
  {"zu",	HB_TAG('Z','U','L',' ')}, 	/* Zulu */
  {"zum",	HB_TAG('L','R','C',' ')},	/* Kumzari */
  {"zza",	HB_TAG('Z','Z','A',' ')},	/* Zazaki */

  /* The corresponding languages IDs for the following IDs are unclear,
   * overlap, or are architecturally weird. Needs more research. */

/*{"chp",	HB_TAG('S','A','Y',' ')},*/	/* Sayisi */
/*{"cwd",	HB_TAG('T','C','R',' ')},*/	/* TH-Cree */
/*{"emk",	HB_TAG('E','M','K',' ')},*/	/* Eastern Maninkakan */
/*{"krc",	HB_TAG('B','A','L',' ')},*/	/* Balkar */
/*{"??",	HB_TAG('B','C','R',' ')},*/	/* Bible Cree */
/*{"zh?",	HB_TAG('C','H','N',' ')},*/	/* Chinese (seen in Microsoft fonts) */
/*{"ar-Syrc?",	HB_TAG('G','A','R',' ')},*/	/* Garshuni */
/*{"hy?",	HB_TAG('H','Y','E','0')},*/	/* Armenian East (ISO 639-3 hye according to Microsoft, but that’s equivalent to ISO 639-1 hy) */
/*{"ga-Latg?/"	HB_TAG('I','R','T',' ')},*/	/* Irish Traditional */
/*{"krc",	HB_TAG('K','A','R',' ')},*/	/* Karachay */
/*{"ka-Geok?",	HB_TAG('K','G','E',' ')},*/	/* Khutsuri Georgian */
/*{"kca",	HB_TAG('K','H','K',' ')},*/	/* Khanty-Kazim */
/*{"kca",	HB_TAG('K','H','S',' ')},*/	/* Khanty-Shurishkar */
/*{"kca",	HB_TAG('K','H','V',' ')},*/	/* Khanty-Vakhi */
/*{"kqs, kss",	HB_TAG('K','I','S',' ')},*/	/* Kisii */
/*{"lua",	HB_TAG('L','U','A',' ')},*/	/* Luba-Lulua */
/*{"mlq",	HB_TAG('M','L','N',' ')},*/	/* Malinke */
/*{"nso",	HB_TAG('N','S','O',' ')},*/	/* Sotho, Northern */
/*{"??",	HB_TAG('M','A','L',' ')},*/	/* Malayalam Traditional */
/*{"csw",	HB_TAG('N','C','R',' ')},*/	/* N-Cree */
/*{"csw",	HB_TAG('N','H','C',' ')},*/	/* Norway House Cree */
/*{"el-polyton",	HB_TAG('P','G','R',' ')},*/	/* Polytonic Greek */
/*{"bgr, cnh, cnw, czt, sez, tcp, csy, ctd, flm, pck, tcz, zom, cmr, dao, hlt, cka, cnk, mrh, mwg, cbl, cnb, csh",	HB_TAG('Q','I','N',' ')},*/	/* Chin */
/*{"??",	HB_TAG('Y','I','C',' ')},*/	/* Yi Classic */
/*{"zh-Latn-pinyin",	HB_TAG('Z','H','P',' ')},*/	/* Chinese Phonetic */
};

typedef struct {
  char language[8];
  hb_tag_t tag;
} LangTagLong;
static const LangTagLong ot_languages_zh[] = {
  {"zh-cn",	HB_TAG('Z','H','S',' ')},	/* Chinese (China) */
  {"zh-hk",	HB_TAG('Z','H','H',' ')},	/* Chinese (Hong Kong) */
  {"zh-mo",	HB_TAG('Z','H','T',' ')},	/* Chinese (Macao) */
  {"zh-sg",	HB_TAG('Z','H','S',' ')},	/* Chinese (Singapore) */
  {"zh-tw",	HB_TAG('Z','H','T',' ')},	/* Chinese (Taiwan) */
  {"zh-hans",	HB_TAG('Z','H','S',' ')},	/* Chinese (Simplified) */
  {"zh-hant",	HB_TAG('Z','H','T',' ')},	/* Chinese (Traditional) */
};

static int
lang_compare_first_component (const char *a,
			      const char *b)
{
  unsigned int da, db;
  const char *p;

  p = strchr (a, '-');
  da = p ? (unsigned int) (p - a) : (unsigned int) strlen (a);

  p = strchr (b, '-');
  db = p ? (unsigned int) (p - b) : (unsigned int) strlen (b);

  return strncmp (a, b, MAX (da, db));
}

static hb_bool_t
lang_matches (const char *lang_str, const char *spec)
{
  unsigned int len = (unsigned int) strlen (spec);

  return strncmp (lang_str, spec, len) == 0 &&
	 (lang_str[len] == '\0' || lang_str[len] == '-');
}

hb_tag_t
hb_ot_tag_from_language (hb_language_t language)
{
  const char *lang_str, *s;

  if (language == HB_LANGUAGE_INVALID)
    return HB_OT_TAG_DEFAULT_LANGUAGE;

  lang_str = hb_language_to_string (language);

  s = strstr (lang_str, "x-hbot");
  if (s) {
    char tag[4];
    int i;
    s += 6;
    for (i = 0; i < 4 && ISALPHA (s[i]); i++)
      tag[i] = TOUPPER (s[i]);
    if (i) {
      for (; i < 4; i++)
	tag[i] = ' ';
      return HB_TAG_CHAR4 (tag);
    }
  }

  /*
   * The International Phonetic Alphabet is a variant tag in BCP-47,
   * which can be applied to any language.
   */
  if (strstr (lang_str, "-fonipa")) {
    return HB_TAG('I','P','P','H');  /* Phonetic transcription—IPA conventions */
  }

  /* Find a language matching in the first component */
  {
    const LangTag *lang_tag;
    lang_tag = (LangTag *) bsearch (lang_str, ot_languages,
				    ARRAY_LENGTH (ot_languages), sizeof (LangTag),
				    (hb_compare_func_t) lang_compare_first_component);
    if (lang_tag)
      return lang_tag->tag;
  }

  /* Otherwise, check the Chinese ones */
  if (0 == lang_compare_first_component (lang_str, "zh"))
  {
    unsigned int i;

    for (i = 0; i < ARRAY_LENGTH (ot_languages_zh); i++)
    {
      const LangTagLong *lang_tag;
      lang_tag = &ot_languages_zh[i];
      if (lang_matches (lang_str, lang_tag->language))
	return lang_tag->tag;
    }

    /* Otherwise just return 'ZHS ' */
    return HB_TAG('Z','H','S',' ');
  }

  s = strchr (lang_str, '-');
  if (!s)
    s = lang_str + strlen (lang_str);
  if (s - lang_str == 3) {
    /* Assume it's ISO-639-3 and upper-case and use it. */
    return hb_tag_from_string (lang_str, s - lang_str) & ~0x20202000u;
  }

  return HB_OT_TAG_DEFAULT_LANGUAGE;
}

/**
 * hb_ot_tag_to_language:
 *
 * 
 *
 * Return value: (transfer none):
 *
 * Since: 0.9.2
 **/
hb_language_t
hb_ot_tag_to_language (hb_tag_t tag)
{
  unsigned int i;

  if (tag == HB_OT_TAG_DEFAULT_LANGUAGE)
    return NULL;

  for (i = 0; i < ARRAY_LENGTH (ot_languages); i++)
    if (ot_languages[i].tag == tag)
      return hb_language_from_string (ot_languages[i].language, -1);

  /* If tag starts with ZH, it's Chinese */
  if ((tag & 0xFFFF0000u)  == 0x5A480000u) {
    switch (tag) {
      case HB_TAG('Z','H','H',' '): return hb_language_from_string ("zh-hk", -1); /* Hong Kong */
      case HB_TAG('Z','H','S',' '): return hb_language_from_string ("zh-Hans", -1); /* Simplified */
      case HB_TAG('Z','H','T',' '): return hb_language_from_string ("zh-Hant", -1); /* Traditional */
      default: break; /* Fall through */
    }
  }

  /* struct LangTag has only room for 3-letter language tags. */
  switch (tag) {
  case HB_TAG('I','P','P','H'):  /* Phonetic transcription—IPA conventions */
    return hb_language_from_string ("und-fonipa", -1);
  }

  /* Else return a custom language in the form of "x-hbotABCD" */
  {
    unsigned char buf[11] = "x-hbot";
    buf[6] = tag >> 24;
    buf[7] = (tag >> 16) & 0xFF;
    buf[8] = (tag >> 8) & 0xFF;
    buf[9] = tag & 0xFF;
    if (buf[9] == 0x20)
      buf[9] = '\0';
    buf[10] = '\0';
    return hb_language_from_string ((char *) buf, -1);
  }
}

#ifdef MAIN
static inline void
test_langs_sorted (void)
{
  for (unsigned int i = 1; i < ARRAY_LENGTH (ot_languages); i++)
  {
    int c = lang_compare_first_component (ot_languages[i-1].language, ot_languages[i].language);
    if (c >= 0)
    {
      fprintf (stderr, "ot_languages not sorted at index %d: %s %d %s\n",
	       i, ot_languages[i-1].language, c, ot_languages[i].language);
      abort();
    }
  }
}

int
main (void)
{
  test_langs_sorted ();
  return 0;
}

#endif
