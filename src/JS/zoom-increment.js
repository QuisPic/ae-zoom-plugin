STR((function (zoomDelta) {
  var viewer = app.activeViewer.views[0];
  var newZoom = viewer.options.zoom * 100 + zoomDelta;
  if (!(zoomDelta % 1)) {
    var epsilon = 0.0000000000001;
    newZoom =
      zoomDelta < 0
        ? Math.ceil(newZoom - epsilon)
        : Math.floor(newZoom + epsilon);
  }
  if (newZoom < 0.8) {
    newZoom = 0.8;
  }
  viewer.options.zoom = newZoom / 100;
}))
