class Presenter implements IPresenter { 
  HashMap<String, ArrayList<PImage>> buffers = new HashMap<String, ArrayList<PImage>>();
  int cm = 0;
  Rect selectRect = null;
  PImage img, bfrimg, dspimg;
  void onSetup() {
    if ( img == null ) { 
      img = loadImage("../../../images/legs.jpg");
      size(img.width, img.height, P3D);
    }
    noLoop();
  }
  
  void onDraw() { 
    println("onDraw");
    background(0);
    image(img, 0, 0);
    if ( selectRect != null ) { 
      fill(color(0, 255, 0, 123));
      rect(selectRect.x, selectRect.y, selectRect.w, selectRect.h);
    }
  }
  void onMouseDragged() { 
    loop();
      println("dragged"+cm);
    if ( cm == 0 ) { 
      if ( selectRect == null ) 
        selectRect = new Rect(mouseX, mouseY, 0, 0);

      selectRect.w = mouseX-selectRect.x;
      selectRect.h = mouseY-selectRect.y;
      
    }
  }
  void onMouseMoved() {}
  void onMouseReleased() { 
    //noLoop();
  }
  void onMousePressed() { 
    println("pressed"+cm);
  }
  void onMouseClicked() { 
    println("clicked "+cm);
    if (  selectRect != null && selectRect.w != 0 && selectRect.h != 0 ) { 
      selectRect.normalize();
      println("still");
      PImage cp = createImage(selectRect.w, selectRect.h, RGB);
      int dx, dy;
      cp.copy(img, dx = selectRect.x, dy = selectRect.y, selectRect.w, selectRect.h, 0, 0, cp.width, cp.height);
      cp = new MockEffect().run(cp, 20.0, 20.0);
      image(cp, selectRect.x, selectRect.y);
      println("encore"+cp.pixels.length);
      noLoop();
      cm = 0; 
      selectRect = null;
    }
  }
}

