interface IEffect {
  PImage run(PImage img, float xp, float yp);
}
interface IConfigurableEffect extends IEffect { 
  PImage run(PImage img, float xp, float yp);
  void config(HashMap<String, Object> conf);
  HashMap<String, Object> config();
  HashMap<String,ConfigDefinition>  defaults(); // key, min, max, default  
}
interface IPresenter { 
  void onSetup();
  void onMouseClicked();
  void onDraw();
  void onMousePressed();
  void onMouseDragged();
  void onMouseReleased();
  void onMouseMoved();
}
interface IDialog { }

interface IControl {
 void onDraw();
 void value(Object o);
 Object value(); 
}
