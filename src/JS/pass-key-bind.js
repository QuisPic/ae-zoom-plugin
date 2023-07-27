STR((function (type, mask, keycode) {
    if ($.global.__zoom_key_capture_object__) {
        $.global.__zoom_key_capture_object__.passFn(type, mask, keycode);
    } 
}))
