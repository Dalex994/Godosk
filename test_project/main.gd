extends Node2D

func _ready() -> void:
	test_plugin()

func test_plugin():
	var my_class:Test = Test.new()
	
	my_class.say_hello()
	
	print(my_class.my_data)
	my_class.my_data = "Yooo !"
	print(my_class.my_data)
