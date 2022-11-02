/*-
 * Copyright (c) 2022, KusaReMKN.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef JEEDO_BNO055_H
#define JEEDO_BNO055_H

#define BNO055_ADDR	0x29
#define BNO055_ADDR_ALT	0x28
#define BNO055_ADDR_HID	0x40

enum {
	CHIP_ID,		/* R: BNO055 CHIP ID */
	ACC_ID,			/* R: ACC chip ID */
	MAG_ID,			/* R: MAG chip ID */
	GYR_ID,			/* R: GYRO chip ID */
	SW_REV_ID_LSB,		/* R: SW Revision ID <7:0> */
	SW_REV_ID_MSB,		/* R: SW Revision ID <15:8> */
	BL_Rev_ID,		/* R: Bootloader Version */
	Page_ID,		/* R: Page ID */
	ACC_DATA_X_LSB,		/* R: Acceleration Data X <7:0> */
	ACC_DATA_X_MSB,		/* R: Acceleration Data X <15:8> */
	ACC_DATA_Y_LSB,		/* R: Acceleration Data Y <7:0> */
	ACC_DATA_Y_MSB,		/* R: Acceleration Data Y <15:8> */
	ACC_DATA_Z_LSB,		/* R: Acceleration Data Z <7:0> */
	ACC_DATA_Z_MSB,		/* R: Acceleration Data Z <15:8> */
	MAG_DATA_X_LSB,		/* R: Magnetometer Data X <7:0> */
	MAG_DATA_X_MSB,		/* R: Magnetometer Data X <15:8> */
	MAG_DATA_Y_LSB,		/* R: Magnetometer Data Y <7:0> */
	MAG_DATA_Y_MSB,		/* R: Magnetometer Data Y <15:8> */
	MAG_DATA_Z_LSB,		/* R: Magnetometer Data Z <7:0> */
	MAG_DATA_Z_MSB,		/* R: Magnetometer Data Z <15:8> */
	GYR_DATA_X_LSB,		/* R: Gyroscope Data X <7:0> */
	GYR_DATA_X_MSB,		/* R: Gyroscope Data X <15:8> */
	GYR_DATA_Y_LSB,		/* R: Gyroscope Data Y <7:0> */
	GYR_DATA_Y_MSB,		/* R: Gyroscope Data Y <15:8> */
	GYR_DATA_Z_LSB,		/* R: Gyroscope Data Z <7:0> */
	GYR_DATA_Z_MSB,		/* R: Gyroscope Data Z <15:8> */
	EUL_Heading_LSB,	/* R: Heading Data <7:0> */
	EUL_Heading_MSB,	/* R: Heading Data <15:8> */
	EUL_Roll_LSB,		/* R: Roll Data <7:0> */
	EUL_Roll_MSB,		/* R: Roll Data <15:8> */
	EUL_Pitch_LSB,		/* R: Pitch Data <7:0> */
	EUL_Pitch_MSB,		/* R: Pitch Data <15:8> */
	QUA_Data_w_LSB,		/* R: Quaternion w Data <7:0> */
	QUA_Data_w_MSB,		/* R: Quaternion w Data <15:8> */
	QUA_Data_x_LSB,		/* R: Quaternion x Data <7:0> */
	QUA_Data_x_MSB,		/* R: Quaternion x Data <15:8> */
	QUA_Data_y_LSB,		/* R: Quaternion y Data <7:0> */
	QUA_Data_y_MSB,		/* R: Quaternion y Data <15:8> */
	QUA_Data_z_LSB,		/* R: Quaternion z Data <7:0> */
	QUA_Data_z_MSB,		/* R: Quaternion z Data <15:8> */
	LIN_Data_X_LSB,		/* R: Linear Acceleration Data X <7:0> */
	LIN_Data_X_MSB,		/* R: Linear Acceleration Data X <15:8> */
	LIN_Data_Y_LSB,		/* R: Linear Acceleration Data Y <7:0> */
	LIN_Data_Y_MSB,		/* R: Linear Acceleration Data Y <15:8> */
	LIN_Data_Z_LSB,		/* R: Linear Acceleration Data Z <7:0> */
	LIN_Data_Z_MSB,		/* R: Linear Acceleration Data Z <15:8> */
	GRV_Data_X_LSB,		/* R: Gravity Vector Data X <7:0> */
	GRV_Data_X_MSB,		/* R: Gravity Vector Data X <15:8> */
	GRV_Data_Y_LSB,		/* R: Gravity Vector Data Y <7:0> */
	GRV_Data_Y_MSB,		/* R: Gravity Vector Data Y <15:8> */
	GRV_Data_Z_LSB,		/* R: Gravity Vector Data Z <7:0> */
	GRV_Data_Z_MSB,		/* R: Gravity Vector Data Z <15:8> */
	TEMP,			/* R: Temperature */
	CALIB_STAT,		/* R: Calibration Status */
	ST_RESULT,		/* R: Self Test Result */
	INT_STA,		/* R: Interrupt Status */
	SYS_CLK_STATUS,		/* R: (r/w?) */
	SYS_STATUS,		/* R: System Status Code */
	SYS_ERR,		/* R: System Error Code */
	UNIT_SEL,		/* R/W: Unit Selection */
	RESERVED_0x3C,		/* Reserved */
	OPR_MODE,		/* R/W: Operation Mode */
	PWR_MODE,		/* R/W: Power Mode */
	SYS_TRIGGER,		/* R/W: System Trigger */
	TEMP_SOURCE,		/* R/W: Temperature Source */
	AXIS_MAP_CONFIG,	/* R/W: Axis map Configuration */
	AXIS_MAP_SIGN,		/* R/W: Axis map sign Configuration */
	RESERVED_0x43,		/* Reserved */
	RESERVED_0x44,		/* Reserved */
	RESERVED_0x45,		/* Reserved */
	RESERVED_0x46,		/* Reserved */
	RESERVED_0x47,		/* Reserved */
	RESERVED_0x48,		/* Reserved */
	RESERVED_0x49,		/* Reserved */
	RESERVED_0x4A,		/* Reserved */
	RESERVED_0x4B,		/* Reserved */
	RESERVED_0x4C,		/* Reserved */
	RESERVED_0x4D,		/* Reserved */
	RESERVED_0x4E,		/* Reserved */
	RESERVED_0x4F,		/* Reserved */
	RESERVED_0x50,		/* Reserved */
	RESERVED_0x51,		/* Reserved */
	RESERVED_0x52,		/* Reserved */
	RESERVED_0x53,		/* Reserved */
	RESERVED_0x54,		/* Reserved */
	ACC_OFFSET_X_LSB,	/* R/W: Accelerometer Offset X <7:0> */
	ACC_OFFSET_X_MSB,	/* R/W: Accelerometer Offset X <15:8> */
	ACC_OFFSET_Y_LSB,	/* R/W: Accelerometer Offset Y <7:0> */
	ACC_OFFSET_Y_MSB,	/* R/W: Accelerometer Offset Y <15:8> */
	ACC_OFFSET_Z_LSB,	/* R/W: Accelerometer Offset Z <7:0> */
	ACC_OFFSET_Z_MSB,	/* R/W: Accelerometer Offset Z <15:8> */
	MAG_OFFSET_X_LSB,	/* R/W: Magnetometer Offset X <7:0> */
	MAG_OFFSET_X_MSB,	/* R/W: Magnetometer Offset X <15:8> */
	MAG_OFFSET_Y_LSB,	/* R/W: Magnetometer Offset Y <7:0> */
	MAG_OFFSET_Y_MSB,	/* R/W: Magnetometer Offset Y <15:8> */
	MAG_OFFSET_Z_LSB,	/* R/W: Magnetometer Offset Z <7:0> */
	MAG_OFFSET_Z_MSB,	/* R/W: Magnetometer Offset Z <15:8> */
	GYR_OFFSET_X_LSB,	/* R/W: Gyroscope Offset X <7:0> */
	GYR_OFFSET_X_MSB,	/* R/W: Gyroscope Offset X <15:8> */
	GYR_OFFSET_Y_LSB,	/* R/W: Gyroscope Offset Y <7:0> */
	GYR_OFFSET_Y_MSB,	/* R/W: Gyroscope Offset Y <15:8> */
	GYR_OFFSET_Z_LSB,	/* R/W: Gyroscope Offset Z <7:0> */
	GYR_OFFSET_Z_MSB,	/* R/W: Gyroscope Offset Z <15:8> */
	ACC_RADIUS_LSB,		/* R/W: Accelerometer Radius */
	ACC_RADIUS_MSB,		/* R/W: Accelerometer Radius */
	MAG_RADIUS_LSB,		/* R/W: Magnetometer Radius */
	MAG_RADIUS_MSB,		/* R/W: Magnetometer Radius */
	RESERVED_0x6B,		/* Reserved */
	RESERVED_0x6C,		/* Reserved */
	RESERVED_0x6D,		/* Reserved */
	RESERVED_0x6E,		/* Reserved */
	RESERVED_0x6F,		/* Reserved */
	RESERVED_0x70,		/* Reserved */
	RESERVED_0x71,		/* Reserved */
	RESERVED_0x72,		/* Reserved */
	RESERVED_0x73,		/* Reserved */
	RESERVED_0x74,		/* Reserved */
	RESERVED_0x75,		/* Reserved */
	RESERVED_0x76,		/* Reserved */
	RESERVED_0x77,		/* Reserved */
	RESERVED_0x78,		/* Reserved */
	RESERVED_0x79,		/* Reserved */
	RESERVED_0x7A,		/* Reserved */
	RESERVED_0x7B,		/* Reserved */
	RESERVED_0x7C,		/* Reserved */
	RESERVED_0x7D,		/* Reserved */
	RESERVED_0x7E,		/* Reserved */
	RESERVED_0x7F,		/* Reserved */
};

#define ACC_DATA_X	ACC_DATA_X_LSB
#define ACC_DATA_Y	ACC_DATA_Y_LSB
#define ACC_DATA_Z	ACC_DATA_Z_LSB
#define MAG_DATA_X	MAG_DATA_X_LSB
#define MAG_DATA_Y	MAG_DATA_Y_LSB
#define MAG_DATA_Z	MAG_DATA_Z_LSB
#define GYR_DATA_X	GYR_DATA_X_LSB
#define GYR_DATA_Y	GYR_DATA_Y_LSB
#define GYR_DATA_Z	GYR_DATA_Z_LSB
#define EUL_Heading	EUL_Heading_LSB
#define EUL_Roll	EUL_Roll_LSB
#define EUL_Pitch	EUL_Pitch_LSB
#define QUA_Data_w	QUA_Data_w_LSB
#define QUA_Data_x	QUA_Data_x_LSB
#define QUA_Data_y	QUA_Data_y_LSB
#define QUA_Data_z	QUA_Data_z_LSB
#define LIN_Data_X	LIN_Data_X_LSB
#define LIN_Data_Y	LIN_Data_Y_LSB
#define LIN_Data_Z	LIN_Data_Z_LSB
#define GRV_Data_X	GRV_Data_X_LSB
#define GRV_Data_Y	GRV_Data_Y_LSB
#define GRV_Data_Z	GRV_Data_Z_LSB
#define ACC_OFFSET_X	ACC_OFFSET_X_LSB
#define ACC_OFFSET_Y	ACC_OFFSET_Y_LSB
#define ACC_OFFSET_Z	ACC_OFFSET_Z_LSB
#define MAG_OFFSET_X	MAG_OFFSET_X_LSB
#define MAG_OFFSET_Y	MAG_OFFSET_Y_LSB
#define MAG_OFFSET_Z	MAG_OFFSET_Z_LSB
#define GYR_OFFSET_X	GYR_OFFSET_X_LSB
#define GYR_OFFSET_Y	GYR_OFFSET_Y_LSB
#define GYR_OFFSET_Z	GYR_OFFSET_Z_LSB
#define ACC_RADIUS	ACC_RADIUS_LSB
#define MAG_RADIUS	MAG_RADIUS_LSB

#endif /* !JEEDO_BNO055_H */
