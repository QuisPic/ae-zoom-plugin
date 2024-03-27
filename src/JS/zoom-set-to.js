STR((function (zoomValue) {
  if (zoomValue < 0.8) {
    zoomValue = 0.8;
  }
  app.activeViewer.views[0].options.zoom = zoomValue / 100;
}))
