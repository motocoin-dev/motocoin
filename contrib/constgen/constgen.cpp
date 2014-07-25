#include <cstdint>
#include <cmath>
#include <iostream>
using std::cout;

#include "../../src/moto-engine.h"

static const int32_t g_InvAngPosK = (int32_t)(1.0f/g_MotoAngPosK);

const float g_FrictionK = 20000.0f;
const float g_WheelMass = 10.0f;
const float g_WheelAngularMass = 0.32f;
const float g_BikeMass = 200.0f;
const float g_BikeAngularMass = 60.5f;
const float g_RotationSpeedFast = 12.0f;
const float g_RotationSpeedSlow = 9.0f;
const float g_HeadMass = 5.0f;
const float dt = 1.0f/500.0f;

int main()
{
	const int32_t g_Friction = (int32_t)(g_FrictionK*dt);
	const uint16_t g_fhh = (uint16_t)(1.0/((1.0f/g_WheelMass + MOTO_WHEEL_R*MOTO_WHEEL_R/g_WheelAngularMass)*dt));
	const int16_t g_WheelK = (int16_t)(10000.0f*dt);
	const int16_t g_WheelK0 = 1000;

	cout << "#define g_InvAngPosK " << (int32_t)(1.0f/g_MotoAngPosK) << "\n";
	cout << "#define g_WheelK " << g_WheelK << "\n";
	cout << "#define g_WheelK0 " << g_WheelK0 << "\n";
	cout << "#define g_MaxSpeed " << (int32_t)(110.0f*dt/g_MotoAngPosK) << "\n";
	cout << "#define g_Acceleration " << (int64_t)(600.0f*dt/g_MotoAngPosK) << "\n";
	cout << "#define g_RotationPeriod " << (int16_t)(0.4f/dt) << "\n";
	cout << "#define g_WheelDist " << (int32_t)(1.7f/g_MotoPosK) << "\n";
	cout << "#define g_BikePos0 " << (int32_t)(0.85f/g_MotoPosK) << "\n";
	cout << "#define g_BikePos1 " << (int32_t)(0.6f/g_MotoPosK) << "\n";
	cout << "#define g_HeadPos " << (int32_t)(1.0f/g_MotoPosK) << "\n";
	cout << "#define g_WheelAngularMass_div_dt " << (int32_t)(g_WheelAngularMass/dt) << "\n";
	cout << "#define g_WheelMass_div_dt " << (int32_t)(g_WheelMass/dt) << "\n";
	cout << "#define g_BikeAngularMass_div_dt " << (int32_t)(g_BikeAngularMass/dt) << "\n";
	cout << "#define g_BikeMass_div_dt " << (int32_t)(g_BikeMass/dt) << "\n";
	cout << "#define g_HeadMass_div_dt " << (int32_t)(g_HeadMass/dt) << "\n";
	cout << "#define g_RotationSpeedFast_mul_g_BikeAngularMass_div_g_MotoAngPosK " << (uint64_t)(g_RotationSpeedFast*g_BikeAngularMass/g_MotoAngPosK) << "\n";
	cout << "#define g_RotationSpeedSlow_mul_BikeAngularMass " << (int32_t)(g_RotationSpeedSlow*g_BikeAngularMass) << "\n";
	cout << "#define g_g_mul_WheelMass_mul_dt_div_PosK " << (int64_t)(-9.8f*g_WheelMass*dt/g_MotoPosK) << "\n";
	cout << "#define g_g_mul_BikeMass_mul_dt_div_PosK " << (int64_t)(-9.8f*g_BikeMass*dt/g_MotoPosK) << "\n";
	cout << "#define g_g_mul_HeadMass_mul_dt_div_PosK " << (int64_t)(-9.8f*g_HeadMass*dt/g_MotoPosK) << "\n";
	cout << "#define g_Friction " << g_Friction << "\n";
	cout << "#define g_fhh " << g_fhh << "\n";
	cout << "#define g_InvMaxBrakingDeltaV " << (int32_t)(1.0f/g_Friction*g_WheelAngularMass/dt) << "\n";
	cout << "#define g_001dt_div_PosK " << (int32_t)(0.01*dt/g_MotoPosK) << "\n";
	cout << "#define g_khgjk " << (uint16_t)(1.0f/g_MotoAngPosK/MOTO_WHEEL_R*g_MotoPosK) << "\n";
	cout << "#define g_8_mul_WheelR_mul_g_MotoPosK_div_g_MotoAngPosK " << (uint16_t)(8*MOTO_WHEEL_R*g_MotoPosK/g_MotoAngPosK) << "\n";
	cout << "#define g_MotoWheelR_PosK_div_AngPosK_fhh " << (uint16_t)(MOTO_WHEEL_R*g_MotoPosK/g_MotoAngPosK*g_fhh) << "\n";
	cout << "#define g_65536WheelR " << (int32_t)(MOTO_WHEEL_R*65536) << "\n";
	cout << "#define g_65536HeadR " << (int32_t)(13000) << "\n";
	cout << "#define g_1_div_65536PosK " << (int32_t)(1.0/(65536*g_MotoPosK)) << "\n";
	cout << "#define g_MotoWheelR_div_PosK " << (int32_t)(MOTO_WHEEL_R*0.997f/g_MotoPosK) << "\n";
	cout << "#define g_InvMaxDeltaRotV " << (int32_t)(1.0f/(g_RotationSpeedSlow*dt)) << "\n";
	cout << "#define g_HeadPosThreshold " << (int64_t)(0.6f*0.6f/(g_MotoPosK*g_MotoPosK)) << "\n";
	cout << "#define g_InvK " << (uint32_t)(g_MotoAngPosK/(g_WheelK*g_MotoPosK*g_MotoPosK)) << "\n";
	cout << "#define g_InvK0 " << (uint32_t)(g_MotoAngPosK/(g_WheelK0*g_MotoPosK*g_MotoPosK)) << "\n";
	cout << "#define g_2WheelR_div_PosK " << (int32_t)(2*MOTO_WHEEL_R/g_MotoPosK) << "\n";
	cout << "#define g_Head_plus_WheelR_div_PosK " << (int32_t)((0.238f + MOTO_WHEEL_R)/g_MotoPosK) << "\n";

	cout << "\nstatic const int32_t g_sin[16385]  = {";

	for (int i = 0; i <= 16384; i++)
	{
		if (i % 200 == 0)
			cout << "\n";
		cout << (int32_t)(2147483647.0*sin(i*6.2831853071795864769/65536.0));
		if (i != 16384)
			cout << ", ";
	}
	cout << "};\n";
}