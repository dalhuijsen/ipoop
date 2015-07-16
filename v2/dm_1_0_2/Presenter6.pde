class Presenter6 implements IPresenter { 
  FxMgr fxMgr;
  void onSetup() { 
    fxMgr = new FxMgr();
    IConfigurableEffect fx = new Subtitle();
    fxMgr.effect= fx;
    fxMgr.img = loadImage("../../../images/girl6.jpg");
    fxMgr.rect = new Rect(0, 0, fxMgr.img.width, fxMgr.img.height);
    background(0);
  } 
  void onMouseClicked() {
  } 
  ConfigDialog dialog;
  void onDraw() {
    PImage im = fxMgr.result();
    if ( im != null ) { 
      background(0);
      image(im, 0, 0);
    }
    if ( dialog == null ) { 
      dialog = new ConfigDialog(fxMgr.effect);
      dialog.onDraw();
    }
  } 
  void onMousePressed() {
  } 
  void onMouseDragged() {
  } 
  void onMouseReleased() {
  } 
  void onMouseMoved() {
  }
}


