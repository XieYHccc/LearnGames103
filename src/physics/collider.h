#pragma once

#include "../basic/component.h"

class Object;
class Collider : public Component {
public:
	Collider(Object* object) : Component(object) {}
	virtual ~Collider() {}


};