//=====================================================================================================
// MadgwickAHRS.h
//=====================================================================================================
//
// Implementation of Madgwick's IMU and AHRS algorithms.
// See: http://www.x-io.co.uk/node/8#open_source_ahrs_and_imu_algorithms
//
// Date			Author          Notes
// 29/09/2011	SOH Madgwick    Initial release
// 02/10/2011	SOH Madgwick	Optimised for reduced CPU load
//
//=====================================================================================================
#ifndef MadgwickAHRS_h
#define MadgwickAHRS_h
#include "mpu_data_type.hpp"
//----------------------------------------------------------------------------------------------------
// Variable declaration

extern volatile float beta;				// algorithm gain
extern volatile float q0, q1, q2, q3;	// quaternion of sensor frame relative to auxiliary frame
float qr0, qr2, qr3, qr1;
float qd0, qd1, qd2, qd3;
float magd_r,magd_p,magd_y;
//---------------------------------------------------------------------------------------------------
// Function declarations
#include <math.h>
//---------------------------------------------------------------------------------------------------
// Definitions

#define sampleFreq	512.0f		// sample frequency in Hz
#define betaDef		0.75	// 2 * proportional gain

//---------------------------------------------------------------------------------------------------
// Variable definitions

volatile float beta = betaDef;								// 2 * proportional gain (Kp)
volatile float q0 = 1.0f, q1 = 0.0f, q2 = 0.0f, q3 = 0.0f;	// quaternion of sensor frame relative to auxiliary frame

//---------------------------------------------------------------------------------------------------
// Function declarations


//====================================================================================================
// Functions

//---------------------------------------------------------------------------------------------------
// AHRS algorithm update

EULER_angle MadgwickAHRSupdate(IMU_data data, MAG_data mag, float dt) {
	EULER_angle angle_e;
	float recipNorm;
	float s0, s1, s2, s3;
	float gx,gy,gz,ax,ay,az,mx,my,mz;
	float qDot1, qDot2, qDot3, qDot4;
	float hx, hy;
	float _2q0mx, _2q0my, _2q0mz, _2q1mx, _2bx, _2bz, _4bx, _4bz, _2q0, _2q1, _2q2, _2q3, _2q0q2, _2q2q3, q0q0, q0q1, q0q2, q0q3, q1q1, q1q2, q1q3, q2q2, q2q3, q3q3;

	gx =  data.Gyro_x/180*PI;
	gy =  data.Gyro_y/180*PI;
	gz =  data.Gyro_z/180*PI;
	ax =  data.Acc_x;
	ay =  data.Acc_y;
	az =  data.Acc_z;

    mx = mag.Mag_x;
    my = mag.Mag_y;
    mz = mag.Mag_z;

//	// Use IMU algorithm if magnetometer measurement invalid (avoids NaN in magnetometer normalisation)
//	if((mx == 0.0f) && (my == 0.0f) && (mz == 0.0f)) {
////		MadgwickAHRSupdateIMU(gx, gy, gz, ax, ay, az);
//		return;
//	}

	// Rate of change of quaternion from gyroscope
	qDot1 = 0.5f * (-q1 * gx - q2 * gy - q3 * gz);
	qDot2 = 0.5f * (q0 * gx + q2 * gz - q3 * gy);
	qDot3 = 0.5f * (q0 * gy - q1 * gz + q3 * gx);
	qDot4 = 0.5f * (q0 * gz + q1 * gy - q2 * gx);

	// Compute feedback only if accelerometer measurement valid (avoids NaN in accelerometer normalisation)
	if(!((ax == 0.0f) && (ay == 0.0f) && (az == 0.0f))) {

		// Normalise accelerometer measurement
		recipNorm = invSqrt(ax * ax + ay * ay + az * az);
		ax *= recipNorm;
		ay *= recipNorm;
		az *= recipNorm;

		// Normalise magnetometer measurement
		recipNorm = invSqrt(mx * mx + my * my + mz * mz);
		mx *= recipNorm;
		my *= recipNorm;
		mz *= recipNorm;

		// Auxiliary variables to avoid repeated arithmetic
		_2q0mx = 2.0f * q0 * mx;
		_2q0my = 2.0f * q0 * my;
		_2q0mz = 2.0f * q0 * mz;
		_2q1mx = 2.0f * q1 * mx;
		_2q0 = 2.0f * q0;
		_2q1 = 2.0f * q1;
		_2q2 = 2.0f * q2;
		_2q3 = 2.0f * q3;
		_2q0q2 = 2.0f * q0 * q2;
		_2q2q3 = 2.0f * q2 * q3;
		q0q0 = q0 * q0;
		q0q1 = q0 * q1;
		q0q2 = q0 * q2;
		q0q3 = q0 * q3;
		q1q1 = q1 * q1;
		q1q2 = q1 * q2;
		q1q3 = q1 * q3;
		q2q2 = q2 * q2;
		q2q3 = q2 * q3;
		q3q3 = q3 * q3;

		// Reference direction of Earth's magnetic field
		hx = mx * q0q0 - _2q0my * q3 + _2q0mz * q2 + mx * q1q1 + _2q1 * my * q2 + _2q1 * mz * q3 - mx * q2q2 - mx * q3q3;
		hy = _2q0mx * q3 + my * q0q0 - _2q0mz * q1 + _2q1mx * q2 - my * q1q1 + my * q2q2 + _2q2 * mz * q3 - my * q3q3;
		_2bx = sqrt(hx * hx + hy * hy);
		_2bz = -_2q0mx * q2 + _2q0my * q1 + mz * q0q0 + _2q1mx * q3 - mz * q1q1 + _2q2 * my * q3 - mz * q2q2 + mz * q3q3;
		_4bx = 2.0f * _2bx;
		_4bz = 2.0f * _2bz;

		// Gradient decent algorithm corrective step
		s0 = -_2q2 * (2.0f * q1q3 - _2q0q2 - ax) + _2q1 * (2.0f * q0q1 + _2q2q3 - ay) - _2bz * q2 * (_2bx * (0.5f - q2q2 - q3q3) + _2bz * (q1q3 - q0q2) - mx) + (-_2bx * q3 + _2bz * q1) * (_2bx * (q1q2 - q0q3) + _2bz * (q0q1 + q2q3) - my) + _2bx * q2 * (_2bx * (q0q2 + q1q3) + _2bz * (0.5f - q1q1 - q2q2) - mz);
		s1 = _2q3 * (2.0f * q1q3 - _2q0q2 - ax) + _2q0 * (2.0f * q0q1 + _2q2q3 - ay) - 4.0f * q1 * (1 - 2.0f * q1q1 - 2.0f * q2q2 - az) + _2bz * q3 * (_2bx * (0.5f - q2q2 - q3q3) + _2bz * (q1q3 - q0q2) - mx) + (_2bx * q2 + _2bz * q0) * (_2bx * (q1q2 - q0q3) + _2bz * (q0q1 + q2q3) - my) + (_2bx * q3 - _4bz * q1) * (_2bx * (q0q2 + q1q3) + _2bz * (0.5f - q1q1 - q2q2) - mz);
		s2 = -_2q0 * (2.0f * q1q3 - _2q0q2 - ax) + _2q3 * (2.0f * q0q1 + _2q2q3 - ay) - 4.0f * q2 * (1 - 2.0f * q1q1 - 2.0f * q2q2 - az) + (-_4bx * q2 - _2bz * q0) * (_2bx * (0.5f - q2q2 - q3q3) + _2bz * (q1q3 - q0q2) - mx) + (_2bx * q1 + _2bz * q3) * (_2bx * (q1q2 - q0q3) + _2bz * (q0q1 + q2q3) - my) + (_2bx * q0 - _4bz * q2) * (_2bx * (q0q2 + q1q3) + _2bz * (0.5f - q1q1 - q2q2) - mz);
		s3 = _2q1 * (2.0f * q1q3 - _2q0q2 - ax) + _2q2 * (2.0f * q0q1 + _2q2q3 - ay) + (-_4bx * q3 + _2bz * q1) * (_2bx * (0.5f - q2q2 - q3q3) + _2bz * (q1q3 - q0q2) - mx) + (-_2bx * q0 + _2bz * q2) * (_2bx * (q1q2 - q0q3) + _2bz * (q0q1 + q2q3) - my) + _2bx * q1 * (_2bx * (q0q2 + q1q3) + _2bz * (0.5f - q1q1 - q2q2) - mz);
		recipNorm = invSqrt(s0 * s0 + s1 * s1 + s2 * s2 + s3 * s3); // normalise step magnitude
		s0 *= recipNorm;
		s1 *= recipNorm;
		s2 *= recipNorm;
		s3 *= recipNorm;

		// Apply feedback step
		qDot1 -= beta * s0;
		qDot2 -= beta * s1;
		qDot3 -= beta * s2;
		qDot4 -= beta * s3;
	}

	// Integrate rate of change of quaternion to yield quaternion
	q0 += qDot1 * dt;
	q1 += qDot2 * dt;
	q2 += qDot3 * dt;
	q3 += qDot4 * dt;

	// Normalise quaternion
	recipNorm = invSqrt(q0 * q0 + q1 * q1 + q2 * q2 + q3 * q3);
	q0 *= recipNorm;
	q1 *= recipNorm;
	q2 *= recipNorm;
	q3 *= recipNorm;


	float sinr = 2*(q0*q1 + q2 * q3);
	float cosr = 1 - 2*(q1*q1 + q2 * q2);
	magd_r = atan2(sinr, cosr);


	float sinp = 2*( q0*q2 - q3*q1);
    if (sinp >= 1)
    	magd_p = PI/2;
    else{
    	if(sinp <= -1){
    	magd_p = -PI/2;
    }
    else{
    	magd_p = asin(sinp);
    }
    }


	float siny = 2*( q0*q3 + q2*q1);
	float cosy = 1 - 2*( q1*q1 + q3*q3);
	magd_y = atan2(siny, cosy);

	angle_e.roll = magd_r*RAD2DEC;
	angle_e.pitch = magd_p*RAD2DEC;
	angle_e.yaw = magd_y*RAD2DEC;
	return angle_e;
}

//---------------------------------------------------------------------------------------------------
// IMU algorithm update

EULER_angle MadgwickAHRSupdateIMU(IMU_data data,float dt) {
	EULER_angle angle_e;

	float recipNorm;
	float s0, s1, s2, s3;
	float gx,gy,gz,ax,ay,az;
	float qDot1, qDot2, qDot3, qDot4;
	float _2q0, _2q1, _2q2, _2q3, _4q0, _4q1, _4q2 ,_8q1, _8q2, q0q0, q1q1, q2q2, q3q3;

	gx =  data.Gyro_x/180*PI;
	gy =  data.Gyro_y/180*PI;
	gz =  data.Gyro_z/180*PI;
	ax =  data.Acc_x;
	ay =  data.Acc_y;
	az =  data.Acc_z;

	// Rate of change of quaternion from gyroscope
	qDot1 = 0.5f * (-q1 * gx - q2 * gy - q3 * gz);
	qDot2 = 0.5f * (q0 * gx + q2 * gz - q3 * gy);
	qDot3 = 0.5f * (q0 * gy - q1 * gz + q3 * gx);
	qDot4 = 0.5f * (q0 * gz + q1 * gy - q2 * gx);

	// Compute feedback only if accelerometer measurement valid (avoids NaN in accelerometer normalisation)
	if(!((ax == 0.0f) && (ay == 0.0f) && (az == 0.0f))) {

		// Normalise accelerometer measurement
		recipNorm = invSqrt(ax * ax + ay * ay + az * az);
		ax *= recipNorm;
		ay *= recipNorm;
		az *= recipNorm;

		// Auxiliary variables to avoid repeated arithmetic
		_2q0 = 2.0f * q0;
		_2q1 = 2.0f * q1;
		_2q2 = 2.0f * q2;
		_2q3 = 2.0f * q3;
		_4q0 = 4.0f * q0;
		_4q1 = 4.0f * q1;
		_4q2 = 4.0f * q2;
		_8q1 = 8.0f * q1;
		_8q2 = 8.0f * q2;
		q0q0 = q0 * q0;
		q1q1 = q1 * q1;
		q2q2 = q2 * q2;
		q3q3 = q3 * q3;

		// Gradient decent algorithm corrective step
		s0 = _4q0 * q2q2 + _2q2 * ax + _4q0 * q1q1 - _2q1 * ay;
		s1 = _4q1 * q3q3 - _2q3 * ax + 4.0f * q0q0 * q1 - _2q0 * ay - _4q1 + _8q1 * q1q1 + _8q1 * q2q2 + _4q1 * az;
		s2 = 4.0f * q0q0 * q2 + _2q0 * ax + _4q2 * q3q3 - _2q3 * ay - _4q2 + _8q2 * q1q1 + _8q2 * q2q2 + _4q2 * az;
		s3 = 4.0f * q1q1 * q3 - _2q1 * ax + 4.0f * q2q2 * q3 - _2q2 * ay;
		recipNorm = invSqrt(s0 * s0 + s1 * s1 + s2 * s2 + s3 * s3); // normalise step magnitude
		s0 *= recipNorm;
		s1 *= recipNorm;
		s2 *= recipNorm;
		s3 *= recipNorm;

		// Apply feedback step
		qDot1 -= beta * s0;
		qDot2 -= beta * s1;
		qDot3 -= beta * s2;
		qDot4 -= beta * s3;
	}

	// Integrate rate of change of quaternion to yield quaternion
	q0 += qDot1 * (dt);
	q1 += qDot2 * (dt);
	q2 += qDot3 * (dt);
	q3 += qDot4 * (dt);

	// Normalise quaternion
	recipNorm = invSqrt(q0 * q0 + q1 * q1 + q2 * q2 + q3 * q3);
	q0 *= recipNorm;
	q1 *= recipNorm;
	q2 *= recipNorm;
	q3 *= recipNorm;


	float sinr = 2*(q0*q1 + q2 * q3);
	float cosr = 1 - 2*(q1*q1 + q2 * q2);
	magd_r = atan2(sinr, cosr);


	float sinp = 2*( q0*q2 - q3*q1);
    if (sinp >= 1)
    	magd_p = PI/2;
    else{
    	if(sinp <= -1){
    	magd_p = -PI/2;
    }
    else{
    	magd_p = asin(sinp);
    }
    }


	float siny = 2*( q0*q3 + q2*q1);
	float cosy = 1 - 2*( q1*q1 + q3*q3);
	magd_y = atan2(siny, cosy);

	angle_e.roll = magd_r*RAD2DEC;
	angle_e.pitch = magd_p*RAD2DEC;
	angle_e.yaw = magd_y*RAD2DEC;
	return angle_e;
}
EULER_angle quaternion_rot(IMU_data data,float dt){
	EULER_angle angle_e;

	float recipNorm;
	float s0, s1, s2, s3;
	float gx,gy,gz,ax,ay,az;
	float qDot1, qDot2, qDot3, qDot4;
	float _2q0, _2q1, _2q2, _2q3, _4q0, _4q1, _4q2 ,_8q1, _8q2, q0q0, q1q1, q2q2, q3q3;

	gx =  data.Gyro_x/180*PI;
	gy =  data.Gyro_y/180*PI;
	gz =  data.Gyro_z/180*PI;
	ax =  data.Acc_x;
	ay =  data.Acc_y;
	az =  data.Acc_z;

	// Rate of change of quaternion from gyroscope
	qDot1 = 0.5f * (-q1 * gx - q2 * gy - q3 * gz);
	qDot2 = 0.5f * (q0 * gx + q2 * gz - q3 * gy);
	qDot3 = 0.5f * (q0 * gy - q1 * gz + q3 * gx);
	qDot4 = 0.5f * (q0 * gz + q1 * gy - q2 * gx);

//	if(!((ax == 0.0f) && (ay == 0.0f) && (az == 0.0f))) {
//
//			// Normalise accelerometer measurement
//			recipNorm = invSqrt(ax * ax + ay * ay + az * az);
//			ax *= recipNorm;
//			ay *= recipNorm;
//			az *= recipNorm;
//
//			// Auxiliary variables to avoid repeated arithmetic
//			_2q0 = 2.0f * q0;
//			_2q1 = 2.0f * q1;
//			_2q2 = 2.0f * q2;
//			_2q3 = 2.0f * q3;
//			_4q0 = 4.0f * q0;
//			_4q1 = 4.0f * q1;
//			_4q2 = 4.0f * q2;
//			_8q1 = 8.0f * q1;
//			_8q2 = 8.0f * q2;
//			q0q0 = q0 * q0;
//			q1q1 = q1 * q1;
//			q2q2 = q2 * q2;
//			q3q3 = q3 * q3;
//
//			// Gradient decent algorithm corrective step
//			s0 = _4q0 * q2q2 + _2q2 * ax + _4q0 * q1q1 - _2q1 * ay;
//			s1 = _4q1 * q3q3 - _2q3 * ax + 4.0f * q0q0 * q1 - _2q0 * ay - _4q1 + _8q1 * q1q1 + _8q1 * q2q2 + _4q1 * az;
//			s2 = 4.0f * q0q0 * q2 + _2q0 * ax + _4q2 * q3q3 - _2q3 * ay - _4q2 + _8q2 * q1q1 + _8q2 * q2q2 + _4q2 * az;
//			s3 = 4.0f * q1q1 * q3 - _2q1 * ax + 4.0f * q2q2 * q3 - _2q2 * ay;
//			recipNorm = invSqrt(s0 * s0 + s1 * s1 + s2 * s2 + s3 * s3); // normalise step magnitude
//			s0 *= recipNorm;
//			s1 *= recipNorm;
//			s2 *= recipNorm;
//			s3 *= recipNorm;
//
//			// Apply feedback step
//			qDot1 -= beta * s0;
//			qDot2 -= beta * s1;
//			qDot3 -= beta * s2;
//			qDot4 -= beta * s3;
//		}

	q0 += qDot1 * (dt);
	q1 += qDot2 * (dt);
	q2 += qDot3 * (dt);
	q3 += qDot4 * (dt);

	recipNorm = invSqrt(q0 * q0 + q1 * q1 + q2 * q2 + q3 * q3);
		q0 *= recipNorm;
		q1 *= recipNorm;
		q2 *= recipNorm;
		q3 *= recipNorm;


		float sinr = 2*(q0*q1 + q2 * q3);
		float cosr = 1 - 2*(q1*q1 + q2 * q2);
		magd_r = atan2(sinr, cosr);


		float sinp = 2*( q0*q2 - q3*q1);
	    if (sinp >= 1)
	    	magd_p = PI/2;
	    else{
	    	if(sinp <= -1){
	    	magd_p = -PI/2;
	    }
	    else{
	    	magd_p = asin(sinp);
	    }
	    }


		float siny = 2*( q0*q3 + q2*q1);
		float cosy = 1 - 2*( q1*q1 + q3*q3);
		magd_y = atan2(siny, cosy);

		angle_e.roll = magd_r*RAD2DEC;
		angle_e.pitch = magd_p*RAD2DEC;
		angle_e.yaw = magd_y*RAD2DEC;
		return angle_e;
}

//---------------------------------------------------------------------------------------------------
// Fast inverse square-root
// See: http://en.wikipedia.org/wiki/Fast_inverse_square_root



#endif
//=====================================================================================================
// End of file
//=====================================================================================================
