// ----------------------------------------------------------------------------------------------------------------
// Control camera
// -----------------------------------------------------------------------------------------------------------------
#ifndef _camera_h_
#define _camera_h_
#include "settings.h"

class camera_c {
	public:
		camera_c();
		void shoot();
		void setPin(bool pinstate);
};

#endif
