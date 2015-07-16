class Orchestrator implements IPresenter { 
  ArrayList<IPresenter> currentStage = new ArrayList<IPresenter>();
  void onSetup() {
    bootstrap();
    for(IPresenter p : currentStage) p.onSetup();
  }
  void onMouseClicked() {
    for(IPresenter p : currentStage) p.onMouseClicked();    
  }
  void onDraw() {
    bootstrap();
    for(IPresenter p : currentStage) p.onDraw();    
  }
  void onMousePressed() {
    for(IPresenter p : currentStage) p.onMousePressed();    
  }
  void onMouseDragged() {
    for(IPresenter p : currentStage) p.onMouseDragged();    
  }
  void onMouseReleased() {
    for(IPresenter p : currentStage) p.onMouseReleased();    
  }
  void onMouseMoved() {
    for(IPresenter p : currentStage) p.onMouseMoved();    
  }

  void bootstrap() { 
    
    if ( state.currentEffect == null ) { 
      IPresenter p = new Presenter8();
      currentStage.add(p);
    } else {
       
    }
  }
}

