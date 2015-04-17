PImage img1;
color bgcolor = color(255,0);
void setup() { 
	background(bgcolor);
	size(2048,1536);
//	frameRate(1);
	noLoop();
}

void draw() { 
	if ( img1 != null ) { 
		background(bgcolor);
		image(img1,0,0);
	}
}

interface IGlitch {
  PImage glitch(PImage img);
} 


interface IMenuRunnable {
  String title();// = "iMenuRunnable";
  String group();
  Boolean saveHistory();// = false;
  Boolean savePreFxImage();// = false;
  void run(int rowidx, int btnidx, int xPerc, int yPerc);
}


/* EXTERNAL FUNCTIONS */
void setImage(String datauri) { 
/*	PImage tmpimg = loadImage(datauri);
	if ( tmpimg.width < 500 || tmpimg.height < 500 ) { 
		img1 = createImage(tmpimg.width*2,tmpimg.height*2,RGB);
	} else { 
		img1 = createImage(tmpimg.width,tmpimg.height,RGB);
	}
	img1.copy(tmpimg,0,0,tmpimg.width,tmpimg.height,0,0,img1.width,img1.height); */
	img1 = loadImage(datauri);

	//redraw();
		background(bgcolor);

	setTimeout(redraw,1000);
}

void menuClick(String menuItem) { 
	if ( img1 == null ) { 
		alert("Sorry, no image loaded to glitch on");
		return;
	}
	
	switch ( menuItem ) { 
	case "vertz":
	btnVertz vert = new btnVertz();
	vert.run(1,1,5,90);
	img1.filter(INVERT); // no idea why it doesn't update if i leave these two out
	img1.filter(INVERT);

	//img1.updatePixels();
	redraw();
	break;
	case "invert":
	img1.filter(INVERT);
	redraw();
	break;
	case "vhs":
	btnSinWav bsw = new btnSinWav();
	bsw.run(1,1,75,75);
	redraw();
	break;
	}
	setTimeout(redraw,300);
}





class btnVertz implements IMenuRunnable {
  String title() { return "vertdu";}
  Boolean saveHistory() { return true; }
  Boolean savePreFxImage() { return true; }
  String logMsg = null;
    String group(){ return "glitch";}

  void run(int rowidx, int btnidx, int xPerc, int yPerc){
    img1 = vertdu(img1,xPerc,yPerc);
  }




PImage vertdu(PImage img, int xp, int yp) { 
  img.loadPixels();
  color last = color(0);
  color dc = img.pixels[0];
  int xw = (int)map(xp,0,100,1,10);
  int yw = 2;//(int)map(yp,0,100,1,10);
  int diff = (int)map(yp,0,100,80,10);
  int[] taken = new int[img.pixels.length];
  for ( int x = 0, w = img.width; x<w; x+= xw ) { 
    for (int y = 0, h = img.height; y<h; y+= yw ) { 
      int pos = x + y * w;
      color c = img.pixels[pos];
      if ( abs(brightness(c) - brightness(last)) > diff ) { 
        dc = c;
      }
      last = c;
      if ( taken[pos] == 0 ) { 
        taken[pos] = 1;
        for (int i = y; i < y+yw; i++) { 
          for (int u = x; u < x+xw; u++) { 
           int p = u + i*w;  
           if ( p > 0 && p < img.pixels.length )  { 
             taken[p] = 1;
             img.pixels[p] = dc;
           }
         }
        }
      } 
    }
  }
  img.updatePixels();
  return img;
}


}






















class btnSinWav implements IMenuRunnable {
  String title() { return "sinwav";}
  Boolean  saveHistory() { return true; }
  Boolean savePreFxImage() { return true; }
  String group(){ return "system";}
  void run(int rowidx, int btnidx, int xPerc, int yPerc) {
    img1 = sinwav(img1,xPerc,yPerc);
  }

PImage sinwav(PImage img, int xp, int yp) { 
  int ctr = 0;  
  img.loadPixels();
  float ylength = map(xp,0,100,1,100);
  float xlength = map(yp,0,100,1,100);
  PImage result = createImage(img.width,img.height,RGB);
  for ( int y =0, h = img.height; y<h; y++) { 
    for ( int x = 0, w = img.width; x<w;x++, ctr++) {
      int pos = x + y * w;
      color c = img.pixels[pos];
      int epos = (int)(( x + sin(y/ylength)*xlength)+ y * w);
      if ( epos < result.pixels.length )
        result.pixels[epos] = c;
      else
        result.pixels[pos] = c;
    }
  }
  result.updatePixels();
  return result;
    
}
}
