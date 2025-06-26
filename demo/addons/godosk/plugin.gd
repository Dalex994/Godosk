@tool
extends EditorPlugin

func _enter_tree():
	
	var stt = Godosk.new()
	print("STT plugin is ready.")

func _exit_tree():
	print("Godosk plugin unloaded")
