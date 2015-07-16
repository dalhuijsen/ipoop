class Presenter5 implements IPresenter { 
  HashMap<String, ArrayList<PImage>> buffers = new HashMap<String, ArrayList<PImage>>();
  int cm = 0;

  PImage img, bfrimg, dspimg;
  void onSetup() {
    if ( state.img == null ) { 
      img = loadImage("../../../images/girl1.jpg");
      //size(img.width, img.height, P3D);
      state.fxData.targetRect = null;
    }
    noLoop();
  }

  void onDraw() { 
    //println("onDraw");
    background(0);
    image(img, 0, 0);
    if ( cm < 3 ) { 
      if ( state.fxData.targetRect != null ) { 
        if ( bfrimg == null ) { 
          fill(color(0, 255, 0, 123));
          rect(state.fxData.targetRect.x, state.fxData.targetRect.y, state.fxData.targetRect.w, state.fxData.targetRect.h);
        } else { 
          image(bfrimg, state.fxData.targetRect.x, state.fxData.targetRect.y);
        }
      }
    }
  }
  void onMouseDragged() { 
    dragRect();
  }
  void dragRect() { 
    if ( state.fxData.targetRect == null ) {
      state.fxData.targetRect = new Rect(mouseX, mouseY, 0, 0);
    }
    state.fxData.targetRect.w = mouseX-state.fxData.targetRect.x;
    state.fxData.targetRect.h = mouseY-state.fxData.targetRect.y;
  }
  void onMouseReleased() { 
    noLoop();
  }
  void onMouseMoved() { 
    if ( state.fxData.targetRect != null 
      && state.fxData.targetRect.x < mouseX 
      && state.fxData.targetRect.y < mouseY 
      && mouseX < state.fxData.targetRect.x+state.fxData.targetRect.w-1 
      && mouseY < state.fxData.targetRect.y+state.fxData.targetRect.h-1 ) { 
      loop();

      float xp =  ( 100 * ( ((float)mouseX-(float)state.fxData.targetRect.x) / (float)state.fxData.targetRect.w ) );
      float yp =  ( 100 * ( ((float)mouseY-(float)state.fxData.targetRect.y) / (float)state.fxData.targetRect.h ) );
      //  println(state.fxData.targetRect.w);
      bfrimg = runCurrentEffect(xp, yp);  


      //  println("relevant moved" + (int)xp + " x " + (int)yp);
    }
  }
  void onMousePressed() {
    loop(); println("pressed");
    if ( state.fxData.targetRect == null ) dragRect();
  }
  int fxidx;
  void onMouseClicked() {
    if ( state.fxData.targetRect != null ) { 
      println("RESET");
      state.fxData.targetRect = null;
      bfrimg = null;
    }
  } 

  void xonMouseClicked() { 

    if (  state.fxData.targetRect != null && state.fxData.targetRect.w != 0 && state.fxData.targetRect.h != 0 ) { 
      state.fxData.targetRect.normalize();
      println("click2");
      PImage cp = runCurrentEffect(1, 1);
      image(cp, state.fxData.targetRect.x, state.fxData.targetRect.y);
      if ( bfrimg != null ) { 
        println("click3");
        // img = createImage(img.width, img.height, RGB);
        img.copy(bfrimg, 0, 0, 
        state.fxData.targetRect.w, 
        state.fxData.targetRect.h, 
        state.fxData.targetRect.x, 
        state.fxData.targetRect.y, 
        state.fxData.targetRect.w, 
        state.fxData.targetRect.h);
        redraw();
      }
      bfrimg = null;
      println("encore"+cp.pixels.length);
      // img = createImage(img.width, img.height, RGB);
      background(0);
      image(img, 0, 0);
      noLoop();
      if ( cm != 3 ) {
        cm = 3;
      } else {
        println("click reset");
        cm = 0;
        state.fxData.targetRect = null;
      }

      //state.fxData.targetRect = null;
    }
  }
  PImage runCurrentEffect(float ix, float iy) { 
    PImage cp = createImage(state.fxData.targetRect.w, state.fxData.targetRect.h, RGB);
    int dx, dy;
    cp.copy(img, dx = state.fxData.targetRect.x, dy = state.fxData.targetRect.y, state.fxData.targetRect.w, state.fxData.targetRect.h, 0, 0, cp.width, cp.height);
    if ( state.currentEffect != null ) 
      cp = state.currentEffect.run(cp, ix, iy);
    else 
      cp = new MockEffect().run(cp, ix, iy);

    return cp;
  }
}

