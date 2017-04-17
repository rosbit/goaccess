#if HAVE_CONFIG_H
#include <config.h>
#endif

#include "settings.h"
#include "geoip.h"
#include "util.h"
#include "ip2region.h"
#include <stdarg.h>

static ip2region_entry ip2rEntry;
static int ip2rLoaded = 0;

/*
static void log(const char *fmt, ...) {
	va_list arg_ptr;

	FILE *fp = fopen("a.log", "a");
	if (fp != NULL) {
		va_start(arg_ptr, fmt);
		vfprintf(fp, fmt, arg_ptr);
		va_end(arg_ptr);
		fclose(fp);
	}
}*/

int is_geoip_resource (void)
{
	return ip2rLoaded;
}

enum {
	COUNTRY_IDX = 0,
	REGION_IDX,
	PROVINCE_IDX,
	CITY_IDX,
	ISP_IDX,
	TOTAL_REGION_PART
};

static int breakRegion(const char *region, char sep, char **substr, int limit)
{
	char *s = (char*)region;
	char *tail;
	int i = 0;

	while (i<limit && *s != '\0') {
		substr[i++] = s;
		tail = strchr((const char*)s, sep);
		if (tail == NULL)
			break;
		*tail++ = '\0';
		s = tail;
	}
	return i;
}

static void setNullValues(char *continent, char *country, char *city)
{
	if (city != NULL)
		strncpy(city, "N/A City, N/A Region", CITY_LEN);
	if (country != NULL)
		strncpy(country, "Unknown", COUNTRY_LEN);
	if (continent != NULL)
		strncpy(continent, "Unknown", CONTINENT_LEN);
}

static void breakEntry(const char *region, char *continent, char *country, char *city)
{
	int foregin = 0;
	char *substr[TOTAL_REGION_PART];
	int count = breakRegion(region, '|', substr, TOTAL_REGION_PART);

	if (count != TOTAL_REGION_PART) {
		setNullValues(continent, country, city);
		return;
	}

	foregin = (strcmp(substr[REGION_IDX], "0") == 0);

	if (city != NULL)
		snprintf(city, CITY_LEN, "%s",  foregin ? "N/A City" : substr[CITY_IDX]);

	if (country != NULL)
		strncpy(country, substr[PROVINCE_IDX], COUNTRY_LEN);
	if (continent != NULL)
		strncpy(continent, substr[COUNTRY_IDX], CONTINENT_LEN);
}

int set_geolocation (char *host, char *continent, char *country, char *city)
{
	int type_ip = 0;
	datablock_entry datablock;

	if (!is_geoip_resource())
		return 1;

	if (invalid_ipaddr(host, &type_ip))
		return 1;

	ip2region_btree_search_string(&ip2rEntry, host, &datablock);
	breakEntry(datablock.region, continent, country, city);

	return 0;
}

void geoip_free (void)
{
	if (ip2rLoaded) {
		ip2region_destroy(&ip2rEntry);
		ip2rLoaded = 0;
	}
}

void geoip_get_city (const char *ip, char *location, GTypeIP type_ip)
{
	if (set_geolocation((char*)ip, NULL, NULL, location) != 0) {
		setNullValues(NULL, NULL, location);
	}
}

void geoip_get_continent (const char *ip, char *location, GTypeIP type_ip)
{
	if (set_geolocation((char*)ip, location, NULL, NULL) != 0) {
		setNullValues(location, NULL, NULL);
	}
}

void geoip_get_country (const char *ip, char *location, GTypeIP type_ip)
{
	if (set_geolocation((char*)ip, NULL, location, NULL) != 0) {
		setNullValues(NULL, location, NULL);
	}
}

void init_geoip (void)
{
	/* open ip2region database */
	if (conf.geoip_database != NULL)
		ip2rLoaded = ip2region_create(&ip2rEntry, (char*)conf.geoip_database);
}

