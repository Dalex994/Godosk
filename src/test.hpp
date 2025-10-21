#ifndef GODOSK_TEST_HPP
#define GODOSK_TEST_HPP

#pragma once
#include "godot_cpp/classes/node.hpp"
#include "godot_cpp/classes/wrapped.hpp"
#include "godot_cpp/variant/string.hpp"

using namespace godot;

class Test: public Node{
	GDCLASS(Test, Node)
protected:
	static void _bind_methods();
private:
	String my_data = "Hi guys";

	String get_my_data() const;
	void set_my_data(const String &new_data);

	void say_hello();
	
};


#endif //GODOSK_TEST_HPP
