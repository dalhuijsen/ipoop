State state;//globals
class State { 
  Class[] allTypes;
  Class[] enumerables = new Class[] { 
    IEffect.class
  }; //Which interfaces to store in cache
  HashMap<String, Class> fxTypes = new HashMap<String, Class>(); //fx gets preferential treatment, sue me
  HashMap<String, IEffect> fx = new HashMap<String, IEffect>(); //idem
  IPresenter[] ctrls = new IPresenter[] { 
    //new Orchestrator(),
    new Presenter5(),  //select
   // new Presenter7(), //configdialog
 //   new Presenter4(), //xypad
    new Presenter8(), //fxlist
  }; // the default view
  PImage img;
  HashMap<String, ArrayList<PImage>> buffers = new HashMap<String, ArrayList<PImage>>();
  int fxidx = 1;
  IEffect currentEffect = null;//new MockEffect();
  FxDialogViewData fxData = new FxDialogViewData(currentEffect);
  color fgColor = color(0,255,0,200);
  color bgColor = color(0);
  color fgMenuColor = color(244,123);
  color hlMenuColor = color(0,255,0,223);
  color hlBgMenuColor = color(23, 123, 12, 113);
  color bgMenuColor = color(0,30,0,123);
  color fgTextColor = color(244,123);
  
}

/* plumbing and setup */
void setup() { 
  state = new State();
  setupEnumeratedTypes();
  noLoop();
  size(1024,768,P3D);
  for ( IPresenter ctrl : state.ctrls ) ctrl.onSetup();
}

void draw() {
 background(state.bgColor); 
  for ( IPresenter ctrl : state.ctrls ) ctrl.onDraw();
}

void mouseClicked() { 
  for ( IPresenter ctrl : state.ctrls ) ctrl.onMouseClicked();
}  
void mousePressed() { 
  for ( IPresenter ctrl : state.ctrls ) ctrl.onMousePressed();
}
void mouseDragged() { 
  for ( IPresenter ctrl : state.ctrls ) ctrl.onMouseDragged();
}
void mouseReleased() { 
  for ( IPresenter ctrl : state.ctrls ) ctrl.onMouseReleased();
}
void mouseMoved() { 
  for ( IPresenter ctrl : state.ctrls ) ctrl.onMouseMoved();
}



/*DEBUG */

class MockEffect implements IEffect { 
  MockEffect() { 
    println("dus");
  }
  ArrayList<String> fxk = new ArrayList<String>();
  PImage run(PImage i, float f1, float f2) { 
    i.filter(INVERT);
    return i;
    //  return state.fx.get(fxk.get(state.fxidx)).run(i, f1, f2);
  }
}

