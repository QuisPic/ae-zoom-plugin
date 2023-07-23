STR((function (zoomSteps) {
  var ZOOM_DELTA = 0.01;
  app.activeViewer.views[0].options.zoom += ZOOM_DELTA * zoomSteps;
}))
