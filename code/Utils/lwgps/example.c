/**
 * This example uses direct processing function
 * to process dummy NMEA data from GPS receiver
 */
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "lwgps.h"

/* GPS handle */
lwgps_t hgps;

/**
 * \brief           Dummy data from GPS receiver
 */
const char gps_rx_data[] =
    ""
    "$GPRMC,183729,A,3907.356,N,12102.482,W,000.0,360.0,080301,015.5,E*6F\r\n"
    "$GPRMB,A,,,,,,,,,,,,V*71\r\n"
    "$GPGGA,183730,3907.356,N,12102.482,W,1,05,1.6,646.4,M,-24.1,M,,*75\r\n"
    "$GPGSA,A,3,02,,,07,,09,24,26,,,,,1.6,1.6,1.0*3D\r\n"
    "$GPGSV,2,1,08,02,43,088,38,04,42,145,00,05,11,291,00,07,60,043,35*71\r\n"
    "$GPGSV,2,2,08,08,02,145,00,09,46,303,47,24,16,178,32,26,18,231,43*77\r\n"
    "$PGRME,22.0,M,52.9,M,51.0,M*14\r\n"
    "$GPGLL,3907.360,N,12102.481,W,183730,A*33\r\n"
    "$PGRMZ,2062,f,3*2D\r\n"
    "$PGRMM,WGS84*06\r\n"
    "$GPBOD,,T,,M,,*47\r\n"
    "$GPRTE,1,1,c,0*07\r\n"
    "$GPRMC,183731,A,3907.482,N,12102.436,W,000.0,360.0,080301,015.5,E*67\r\n"
    "$GPRMB,A,,,,,,,,,,,,V*71\r\n";

const char gps_rx_data2[] =
    ""
    "$BDRMC,023656.00,A,2240.61563,N,11359.86512,E,0.16,,140324,,,A,V*2C\r\n"
    "$BDVTG,,,,,0.16,N,0.30,K,A*2F\r\n"
    "$BDGGA,023656.00,2240.61563,N,11359.86512,E,1,23,0.7,96.53,M,-3.52,M,*5B\r\n"
    "$BDGSA,A,3,01,02,03,04,05,06,07,09,10,16,19,20,1.0,0.7,0.8,4*30\r\n"
    "$BDGSA,A,3,22,27,28,30,36,37,39,40,46,59,60,1.0,0.7,0.8,4*3E\r\n"
    "$BDGSV,6,1,23,01,45,125,38,02,46,235,40,03,61,189,46,04,32,112,37,1*7B\r\n"
    "$BDGSV,6,2,23,05,23,254,39,06,84,047,45,07,71,291,44,09,73,346,43,1*75\r\n"
    "$BDGSV,6,3,23,10,61,266,42,16,80,095,45,19,20,251,35,20,13,202,42,1*7D\r\n"
    "$BDGSV,6,4,23,22,06,299,37,27,59,074,44,28,18,042,41,30,53,183,47,1*70\r\n"
    "$BDGSV,6,5,23,36,25,315,43,37,30,086,40,39,72,130,45,40,74,323,46,1*73\r\n"
    "$BDGSV,6,6,23,46,55,012,45,59,49,130,41,60,42,238,44,1*4B\r\n"
    "$BDGSV,3,1,11,19,20,251,32,20,13,202,40,22,06,299,34,27,59,074,43,3*72\r\n"
    "$BDGSV,3,2,11,28,18,042,38,30,53,183,41,36,25,315,38,37,30,086,35,3*73\r\n"
    "$BDGSV,3,3,11,39,72,130,43,40,74,323,43,46,55,012,42,3*48\r\n"
    "$BDGLL,2240.61563,N,11359.86512,E,023656.00,A,A*78\r\n"
    "$BDZDA,023656.00,14,03,2024,00,00*71$GPTXT,01,01,01,ANTENNA OPEN*25\r\n";

const char gps_rx_data3[] =
    "$GNGGA,152900.000,,,,,0,00,25.5,,,,,,*75\r\n"
    "$GNGLL,,,,,152900.000,V,N*6B\r\n"
    "$GNGSA,A,1,,,,,,,,,,,,,25.5,25.5,25.5,1*01\r\n"
    "$GNGSA,A,1,,,,,,,,,,,,,25.5,25.5,25.5,4*04\r\n"
    "$GPGSV,1,1,01,29,,,34,0*68\r\n"
    "$BDGSV,1,1,00,0*74\r\n"
    "$GNRMC,152900.000,V,,,,,,,260126,,,N,V*27\r\n"
    "$GNVTG,,,,,,,,,N*2E\r\n"
    "$GNZDA,152900.000,26,01,2026,00,00*44\r\n"
    "$GPTXT,01,01,01,ANTENNA OPEN*25\r\n";

// $GNGGA,152900.000,,,,,0,00,25.5,,,,,,*75\r\n
// $GNGLL,,,,,152900.000,V,N*6B\r\n
// $GNGSA,A,1,,,,,,,,,,,,,25.5,25.5,25.5,1*01\r\n
// $GNGSA,A,1,,,,,,,,,,,,,25.5,25.5,25.5,4*04\r\n
// $GPGSV,1,1,01,29,,,34,0*68\r\n
// $BDGSV,1,1,00,0*74\r\n
// $GNRMC,152900.000,V,,,,,,,260126,,,N,V*27\r\n
// $GNVTG,,,,,,,,,N*2E\r\n
// $GNZDA,152900.000,26,01,2026,00,00*44\r\n
// $GPTXT,01,01,01,ANTENNA OPEN*25\r\n
/*
$GNGGA,171000.000,2243.78439,N,11415.59140,E,1,08,1.4,7.4,M,-2.6,M,,*5D
$GNGLL,2243.78439,N,11415.59140,E,171000.000,A,A*4F
$GNGSA,A,3,05,13,15,24,29,,,,,,,,3.4,1.4,3.1,1*3F
$GNGSA,A,3,14,33,43,,,,,,,,,,3.4,1.4,3.1,4*36
$GPGSV,3,1,09,05,33,067,16,11,13,139,,13,34,033,23,15,57,002,19,0*6C
$GPGSV,3,2,09,18,44,324,,21,10,085,,23,22,309,,24,53,162,12,0*63
$GPGSV,3,3,09,29,33,223,20,0*56
$BDGSV,1,1,04,14,51,343,17,21,06,059,,33,43,303,17,43,60,239,15,0*76
$GNRMC,171000.000,A,2243.78439,N,11415.59140,E,1.99,342.52,260126,,,A,V*07
$GNVTG,342.52,T,,M,1.16,N,2.15,K,A*21
$GNZDA,171000.000,26,01,2026,00,00*4C
$GPTXT,01,01,01,ANTENNA OPEN*25\r\n

$GNGGA,170958.000,2243.39146,N,11415.83796,E,1,08,1.4,7.6,M,-2.6,M,,*51
$GNGLL,2243.39146,N,11415.83796,E,170958.000,A,A*41
$GNGSA,A,3,05,13,15,24,29,,,,,,,,3.4,1.4,3.1,1*3F
$GNGSA,A,3,14,33,43,,,,,,,,,,3.4,1.4,3.1,4*36
$GPGSV,3,1,09,05,33,067,16,11,13,139,,13,34,033,23,15,57,002,19,0*6C
$GPGSV,3,2,09,18,44,324,,21,10,085,,23,22,309,,24,53,162,12,0*63
$GPGSV,3,3,09,29,33,223,20,0*56
$BDGSV,1,1,04,14,51,343,17,21,06,059,,33,43,303,17,43,60,239,15,0*76
$GNRMC,170958.000,A,2243.39146,N,11415.83796,E,0.64,342.52,260126,,,A,V*0D
$GNVTG,342.52,T,,M,0.64,N,1.19,K,A*2A
$GNZDA,170958.000,26,01,2026,00,00*49
$GPTXT,01,01,01,ANTENNA OPEN*25\r\n
*/

const char gps_rx_data4[] =
    "$GNGGA,170958.000,2243.78416,N,11415.59102,E,1,08,1.4,7.6,M,-2.6,M,,*51\r\n"
    "$GNGLL,2243.78416,N,11415.59102,E,170958.000,A,A*41\r\n"
    "$GNGSA,A,3,05,13,15,24,29,,,,,,,,3.4,1.4,3.1,1*3F\r\n"
    "$GNGSA,A,3,14,33,43,,,,,,,,,,3.4,1.4,3.1,4*36\r\n"
    "$GPGSV,3,1,09,05,33,067,16,11,13,139,,13,34,033,23,15,57,002,19,0*6C\r\n"
    "$GPGSV,3,2,09,18,44,324,,21,10,085,,23,22,309,,24,53,162,12,0*63\r\n"
    "$GPGSV,3,3,09,29,33,223,20,0*56\r\n"
    "$BDGSV,1,1,04,14,51,343,17,21,06,059,,33,43,303,17,43,60,239,15,0*76\r\n"
    "$GNRMC,170958.000,A,2243.78416,N,11415.59102,E,0.64,342.52,260126,,,A,V*0D\r\n"
    "$GNVTG,342.52,T,,M,0.64,N,1.19,K,A*2A\r\n"
    "$GNZDA,170958.000,26,01,2026,00,00*49\r\n"
    "$GPTXT,01,01,01,ANTENNA OPEN*25\r\n";

const char gps_rx_data5[] =
    "$GNGGA,170958.000,2243.55838,N,11415.54018,E,1,08,1.4,7.6,M,-2.6,M,,*51\r\n"
    "$GNGLL,2243.55838,N,11415.54018,E,170958.000,A,A*41\r\n"
    "$GNGSA,A,3,05,13,15,24,29,,,,,,,,3.4,1.4,3.1,1*3F\r\n"
    "$GNGSA,A,3,14,33,43,,,,,,,,,,3.4,1.4,3.1,4*36\r\n"
    "$GPGSV,3,1,09,05,33,067,16,11,13,139,,13,34,033,23,15,57,002,19,0*6C\r\n"
    "$GPGSV,3,2,09,18,44,324,,21,10,085,,23,22,309,,24,53,162,12,0*63\r\n"
    "$GPGSV,3,3,09,29,33,223,20,0*56\r\n"
    "$BDGSV,1,1,04,14,51,343,17,21,06,059,,33,43,303,17,43,60,239,15,0*76\r\n"
    "$GNRMC,170958.000,A,2243.55838,N,11415.54018,E,0.64,342.52,260126,,,A,V*0D\r\n"
    "$GNVTG,342.52,T,,M,0.64,N,1.19,K,A*2A\r\n"
    "$GNZDA,170958.000,26,01,2026,00,00*49\r\n"
    "$GPTXT,01,01,01,ANTENNA OPEN*25\r\n";

// WGS84转GCJ02辅助函数 - 纬度转换
static double Transform_Lat(double x, double y)
{
	double ret = -100.0 + 2.0 * x + 3.0 * y + 0.2 * y * y + 0.1 * x * y + 0.2 * sqrt(fabs(x));
	ret += (20.0 * sin(6.0 * x * 3.1415926535897932384626) + 20.0 * sin(2.0 * x * 3.1415926535897932384626)) * 2.0 / 3.0;
	ret += (20.0 * sin(y * 3.1415926535897932384626) + 40.0 * sin(y / 3.0 * 3.1415926535897932384626)) * 2.0 / 3.0;
	ret += (160.0 * sin(y / 12.0 * 3.1415926535897932384626) + 320 * sin(y * 3.1415926535897932384626 / 30.0)) * 2.0 / 3.0;
	return ret;
}

// WGS84转GCJ02辅助函数 - 经度转换
static double Transform_Lon(double x, double y)
{
	double ret = 300.0 + x + 2.0 * y + 0.1 * x * x + 0.1 * x * y + 0.1 * sqrt(fabs(x));
	ret += (20.0 * sin(6.0 * x * 3.1415926535897932384626) + 20.0 * sin(2.0 * x * 3.1415926535897932384626)) * 2.0 / 3.0;
	ret += (20.0 * sin(x * 3.1415926535897932384626) + 40.0 * sin(x / 3.0 * 3.1415926535897932384626)) * 2.0 / 3.0;
	ret += (150.0 * sin(x / 12.0 * 3.1415926535897932384626) + 300.0 * sin(x / 30.0 * 3.1415926535897932384626)) * 2.0 / 3.0;
	return ret;
}

/**
 * @brief WGS84坐标转换为GCJ02坐标（火星坐标系）
 * @param wgs_lon WGS84经度
 * @param wgs_lat WGS84纬度
 * @param gcj_lon 输出GCJ02经度
 * @param gcj_lat 输出GCJ02纬度
 * @note 用于在中国地图服务（如高德、腾讯）上显示GPS坐标
 */
void WGS84_To_GCJ02(double wgs_lon, double wgs_lat, double *gcj_lon, double *gcj_lat)
{
	double a = 6378245.0;  // 长半轴
	double ee = 0.00669342162296594323;  // 偏心率平方
	
	double dLat = Transform_Lat(wgs_lon - 105.0, wgs_lat - 35.0);
	double dLon = Transform_Lon(wgs_lon - 105.0, wgs_lat - 35.0);
	double radLat = wgs_lat / 180.0 * 3.1415926535897932384626;
	double magic = 1 - ee * sin(radLat) * sin(radLat);
	double sqrtMagic = sqrt(magic);
	dLat = (dLat * 180.0) / ((a * (1 - ee)) / (magic * sqrtMagic) * 3.1415926535897932384626);
	dLon = (dLon * 180.0) / (a / sqrtMagic * cos(radLat) * 3.1415926535897932384626);
	
	*gcj_lat = wgs_lat + dLat;
	*gcj_lon = wgs_lon + dLon;
}

int main()
{
	/* Init GPS */
	lwgps_init(&hgps);

	/* Process all input data */
	lwgps_process(&hgps, gps_rx_data5, strlen(gps_rx_data5));

	/* Print messages */
	printf("========== GPS 原始数据 (WGS84) ==========\r\n");
	printf("Valid status: %d\r\n", hgps.is_valid);
	printf("Latitude: %.6f degrees\r\n", hgps.latitude);
	printf("Longitude: %.6f degrees\r\n", hgps.longitude);
	printf("Altitude: %f meters\r\n", hgps.altitude);
	// printf("Speed: %f m/s\r\n", lwgps_to_speed(hgps.speed, LWGPS_SPEED_MPS));
	printf("Speed: %f knots\r\n", hgps.speed);
	printf("Course: %f degrees\r\n", hgps.course);
	printf("Date: %02d/%02d/%02d\r\n", hgps.date, hgps.month, hgps.year);
	printf("Time: %02d:%02d:%02d\r\n", (hgps.hours + 8) % 24, hgps.minutes, hgps.seconds);
	printf("Fix: %d\r\n", hgps.fix);
	printf("Sats in use: %d\r\n", hgps.sats_in_use);
	
	/* 坐标转换验证 */
	printf("\r\n========== 坐标转换 (WGS84 -> GCJ02) ==========\r\n");
	double wgs_lon = (double)hgps.longitude;
	double wgs_lat = (double)hgps.latitude;
	double gcj_lon, gcj_lat;
	
	WGS84_To_GCJ02(wgs_lon, wgs_lat, &gcj_lon, &gcj_lat);
	
	printf("WGS84坐标:  经度=%.6f, 纬度=%.6f\r\n", wgs_lon, wgs_lat);
	printf("GCJ02坐标:  经度=%.6f, 纬度=%.6f\r\n", gcj_lon, gcj_lat);
	printf("坐标偏移:   经度=%.6f, 纬度=%.6f\r\n", gcj_lon - wgs_lon, gcj_lat - wgs_lat);
	
	/* 验证转换结果 */
	printf("\r\n========== 转换验证 ==========\r\n");
	printf("预期坐标:   经度=114.263970, 纬度=22.723190\r\n");
	printf("WGS84坐标:  经度=%.6f, 纬度=%.6f (偏移: 经度=%.6f, 纬度=%.6f)\r\n", 
		wgs_lon, wgs_lat, wgs_lon - 114.263970, wgs_lat - 22.723190);
	printf("GCJ02坐标:  经度=%.6f, 纬度=%.6f (偏移: 经度=%.6f, 纬度=%.6f)\r\n", 
		gcj_lon, gcj_lat, gcj_lon - 114.263970, gcj_lat - 22.723190);
	
	/* 计算距离误差（米） */
	double lat_diff = (gcj_lat - 22.723190) * 111000.0;  // 1度纬度约111km
	double lon_diff = (gcj_lon - 114.263970) * 111000.0 * cos(wgs_lat * 3.1415926535897932384626 / 180.0);
	double distance_error = sqrt(lat_diff * lat_diff + lon_diff * lon_diff);
	printf("GCJ02距离误差: %.2f 米\r\n", distance_error);

	return 0;
}
