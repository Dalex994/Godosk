release:
	scons target=template_release symbols_visibility=hidden optimize=speed generate_bindings=yes precision=double

debug:
	scons target=template_debug platform=windows symbols_visibility=visible optimize=debug generate_bindings=yes precision=double