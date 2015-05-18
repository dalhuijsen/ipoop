//import gab.opencv.*;
import java.awt.Rectangle;
//import org.processing.wiki.triangulate.*;
String copyright = "Beta Software - Copyright (C)2011-2015 <http://www.applesandchickens.com>";

PImage img1,_img1,pimg1,bimg1;

PFont fnt1;
ArrayList<PImage> history = new ArrayList<PImage>();
ArrayList<String> logBuffer = new ArrayList<String>();
//  String[] names = { "THRESHOLD", "GRAY", "OPAQUE", "INVERT", "POSTERIZE", "BLUR", "ERODE", "DILATE" };
HashMap<String,String> cmdAlias = new HashMap<String,String>();
//cmdAlias.add("Vert",ant"vertdu");
int historyIdx = 0;
String imgPath = "", imgName = "";
String[][] buttonText = {
// web layout:
//  {"vertdu","","","","","","","spielerei","","","","","",""},
//  {"","","","","","","","spielerei","","","","","","open"}
  
// standard:
  { "open",      "save",    "blendmode",      "undo",      "redo",      "reset",        "background",              "",        "doublex",     "fltThresh",  "","",  "fltErode",    ""},
  { "ymirror",   "mirror",   "sinwav",      "brDrift",            "sinsin",   "sinsin2",   "sinsin3",   "diag",    "doubley",            "fltGray",  "","",      "fltDilate",    "" },
  { "lSort",     "vertdu",    "hordu",   "lameGlitch",  "echo1",      "switchcolor",       "superlame",     "transbuffer",        "dubsiz",            "fltOpaque",     "","", "jarimg",    "slicer"}, 
  {"hibufcol",    "dus",   "edge000",   "bw",      "interlace",  "buffer",      "rgboffset",     "poster",  "halfsiz",            "fltInvert",   "","",   "",    "stretch"},
  {"vBlur",      "selfie",/*echo2*/    "contrast",/*echo3*/ "warp",        "blendbfr",     "blendbuffer", "offset",        "lameglitch2","halfx",            "fltPoster",    "","",  "",    "stretch2"},
  {"",       "",         "",    "",       "",         "",    "brighter",      "edged",        "halfy",            "fltBlur",      "lacebuf","",  "",    "stretch3"},
};
String fontName = "Consolas";
int buttonCount = buttonText[0].length, buttonRowCount = buttonText.length;
int frameWidth = 1024, leftMargin = 0, topMargin = 0, menuHeight = 200,  imgX = 0, imgY = 0, imgW = 0, imgH = 0, pos = 0, btnWidth = 0, btnHeight = 0;
float phi = 1.618;
int blendMode = DARKEST;
int blendIdx = 0;
int[] blendModes = { DARKEST, BLEND, ADD, SUBTRACT,  LIGHTEST, DIFFERENCE, EXCLUSION, MULTIPLY, SCREEN, OVERLAY, HARD_LIGHT, SOFT_LIGHT, DODGE, BURN };
String[] blendTitles = { "DARKEST","BLEND", "ADD", "SUBTRACT",  "LIGHTEST", "DIFFERENCE", "EXCLUSION", "MULTIPLY", "SCREEN", "OVERLAY", "HARD_LIGHT", "SOFT_LIGHT", "DODGE", "BURN" };
  int[] fils = { THRESHOLD, GRAY, OPAQUE, INVERT, POSTERIZE, BLUR, ERODE, DILATE };
  String[] filTitles = { "THRESHOLD", "GRAY", "OPAQUE", "INVERT", "POSTERIZE", "BLUR", "ERODE", "DILATE" };
  int filidx = -1;
int xymode = 0;
int xymodeIdx = 0;
String[] xymodes = { "command" , "insert" };
int pxWidth = 1, pxHeight = 1, sensitivity = 10, xPercent = 0, yPercent = 0; //user settable

int maxPxHeight = 50;
int maxPxWidth = 50;
int r1=(int)(random(1)*80), r2=(int)(random(1)*40), r3=(int)(180+random(1)*75);
color fgLogColor=color(255,255,255,128);
color bgLogColor=color(20,30,40,88);
color bgMenuColor=color(r1,r1,r1,160);
color bgColor=color(r2,r2,r2,180);
color fgMenuColor=color(r3,255-r2,255-r1,153);
  ArrayList<PImage> buffer;

HashMap<String,IMenuRunnable> menuItems = new HashMap<String,IMenuRunnable>();
interface IGlitch {
  PImage glitch(PImage img);
} 

void setup() { 
  /*PImage test = loadImage("../../../images/legs.jpg");
  size(test.width, test.height);//, RGB);
  image(sinsin(test,0,0),0,0);  */
  
  size(frameWidth,768);
  fnt1 = createFont(fontName,16,true);
  if ( typeof(frame) != "undefined" && frame != null ) //FIXME: catch this somehow so it doesn't get executed in jscript mode
    frame.setResizable(true);
//  noLoop();
  frameRate(1); 
  setMenuHandlers();

}

void setImage(String datauri) { 
/*	PImage tmpimg = loadImage(datauri);
	if ( tmpimg.width < 500 || tmpimg.height < 500 ) { 
		img1 = createImage(tmpimg.width*2,tmpimg.height*2,RGB);
	} else { 
		img1 = createImage(tmpimg.width,tmpimg.height,RGB);
	}
	img1.copy(tmpimg,0,0,tmpimg.width,tmpimg.height,0,0,img1.width,img1.height);
 */
	img1 = loadImage(datauri);
	_img1 = img1;

 Log("Opening '"+imgName+"' from '"+imgPath+"'"); 
    fileIsLoaded();
	//redraw();
		background(bgcolor);

	setTimeout(redraw,3000);
}

void draw() { 
  loadPixels();
  background(bgColor);
  drawMenu();
  drawMenu2();
  if ( img1 != null ) { 
    drawImage(img1);
  } 
  drawLog();
  drawStats();
}




void setMenuHandlers() { 
  menuItems.put("open",new btnOpen());  
  menuItems.put("mode",new btnMode());  
  menuItems.put("blendmode",new btnBlendMode());  
  menuItems.put("revert",new btnRevert());  
  menuItems.put("reset",new btnReset());  
  menuItems.put("save",new btnSave());  
    menuItems.put("reset",new btnReset());  

  menuItems.put("undo",new btnUndo());  
  menuItems.put("redo",new btnRedo());  
  menuItems.put("rstPrev",new btnResetPrev());  
  menuItems.put("vert",new btnVert());  
  menuItems.put("pxlWidth",new btnPxWidth());  
  menuItems.put("pxlHeight",new btnPxHeight());  
  menuItems.put("sensitivity",new btnSensitivity());  
 
  menuItems.put("lSort",new btnLameSort());  
  menuItems.put("blendSauce",new btnBlendSauce());  
  menuItems.put("blendPrev",new btnBlendPrev());  
  menuItems.put("lameGlitch",new btnLameGlitch());  
  menuItems.put("lameglitch2",new btnLameGlitch2());  
  //menuItems.put("redraw",new btnRedraw());  

  menuItems.put("edge000",new btnEdge000());  
  menuItems.put("edge002",new btnEdge002());  
  menuItems.put("interlace",new btnInterlace());  
  menuItems.put("vBlur",new btnVertBlur());  
  
  menuItems.put("exit",new btnExit());  
  menuItems.put("echo1", new btnEcho1());
  menuItems.put("echo2", new btnEcho1(1));
  menuItems.put("echo3", new btnEcho1(2));
  menuItems.put("vertdu", new btnVertdu());
  menuItems.put("hordu", new btnHordu());
  menuItems.put("mirror", new btnMirror());
  menuItems.put("ymirror", new btnYMirror());
  menuItems.put("bw", new btnBw());
  menuItems.put("buffer", new btnBuffer());
  menuItems.put("contrast", new btnContrast());
  menuItems.put("blendbuffer", new btnBlendBuffer());
  menuItems.put("warp", new btnWarp());
  menuItems.put("offset", new btnOffset());
  menuItems.put("brighter", new btnBrighter());
  menuItems.put("rgboffset", new btnRgbOffset());
  menuItems.put("faces", new btnFaces());
  menuItems.put("superlame", new btnSuperLame());
  menuItems.put("diag", new btnDiag());
  menuItems.put("poster", new btnPoster());
  menuItems.put("filter", new btnFilter());
  menuItems.put("fltThresh", new btnFilter(THRESHOLD));
  menuItems.put("fltGray", new btnFilter(GRAY));
  menuItems.put("fltOpaque", new btnFilter(OPAQUE));
  menuItems.put("fltInvert", new btnFilter(INVERT));
  menuItems.put("fltPoster", new btnFilter(POSTERIZE));
  menuItems.put("fltBlur", new btnFilter(BLUR));
  menuItems.put("fltErode", new btnFilter(ERODE));
  menuItems.put("fltDilate", new btnFilter(DILATE));
  menuItems.put("stretch", new btnStretch(0));
  menuItems.put("stretch2", new btnStretch(1));
  menuItems.put("stretch3", new btnStretch(2));
  menuItems.put("sinsin",new btnSinsin());  
  menuItems.put("sinsin2",new btnSinsin(1));  
  menuItems.put("sinsin3",new btnSinsin(2));  
  menuItems.put("doublex",new btnDoubleX(0));  
  menuItems.put("doubley",new btnDoubleX(1));  
  menuItems.put("dubsiz",new btnResize(2,2));  
  menuItems.put("halfsiz",new btnResize(0.5,0.5));  
  menuItems.put("halfx",new btnResize(0.5,1));  
  menuItems.put("halfy",new btnResize(1,0.5));  
  menuItems.put("background",new btnBg());  
  menuItems.put("jarimg",new btnJar());  
  menuItems.put("slicer",new btnSlicer());  
  menuItems.put("brDrift",new btnBrDrift());  
  menuItems.put("transbuffer",new btnTransBuffer());  
  menuItems.put("selfie",new btnSelfie());  
  menuItems.put("edged",new btnEdged());  
  menuItems.put("spielerei",new btnSpielerei());  
  menuItems.put("switchcolor",new btnSwitchColor());  
  menuItems.put("dus",new btnDus());  
  menuItems.put("hibufcol",new btnHiBufCol());  
  menuItems.put("blendbfr",new btnBlendImageBuffer());
  menuItems.put("horiz3",new btnHoriz3());
  menuItems.put("sinwav",new btnSinWav());
  menuItems.put("terminator",new btnTerminator());
  menuItems.put("VHS",new btnVhs());
  menuItems.put("Macro",new btnMacro());
  menuItems.put("lacebuf",new btnLaceBuf());
  menuItems.put("macroo",new btnMacroo());
}


class btnTemplate implements IMenuRunnable {
  String title() { return "template";}
  Boolean  saveHistory() { return true; }
  Boolean savePreFxImage() { return true; }
  String group(){ return "system";}
  void run(int rowidx, int btnidx, int xPerc, int yPerc) {
   // img1 = dus(img1,xPerc,yPerc);
  }
}

class btnMacroo implements IMenuRunnable {
  String title() { return "macroo";}
  Boolean  saveHistory() { return true; }
  Boolean savePreFxImage() { return true; }
  String group(){ return "system";}
  void run(int rowidx, int btnidx, int xPerc, int yPerc) {
    loadTextFile();
   // img1 = dus(img1,xPerc,yPerc);
  }
}


class btnLaceBuf implements IMenuRunnable {
  String title() { return "lacebuf";}
  Boolean  saveHistory() { return true; }
  Boolean savePreFxImage() { return true; }
  String group(){ return "glitch";}
  void run(int rowidx, int btnidx, int xPerc, int yPerc) {
    if ( buffer.size() > 0 ) {
      img1 = interlaceImages(img1,buffer.get(buffer.size()-1)); 
    }
   // img1 = dus(img1,xPerc,yPerc);
  }
}



PImage interlaceImages(PImage im1, PImage im2) { 
 im1.loadPixels();
 PImage result = createImage(im1.width, im1.height, RGB);
 for ( int y = 0, h = im1.height; y<h; y++) {
   for ( int x = 0, w = im1.width; x<w; x++) { 
     int y2 = (int)map(y,0,h,0,im2.height);
     int x2 = (int)map(x,0,w,0,im2.width);
     int p1 = x + y * w;
     int p2 = x2 + y2 * im2.width;
     color c = y % 2 == 0 ? im1.pixels[p1] : im2.pixels[p2];
     result.pixels[p1] = c;
   }
 }
 result.updatePixels();
 return result;
  
}
class btnMacro implements IMenuRunnable {
  String title() { return "macro";}
  Boolean  saveHistory() { return true; }
  Boolean savePreFxImage() { return true; }
  String group(){ return "system";}
  void run(int rowidx, int btnidx, int xPerc, int yPerc) {
   // img1 = dus(img1,xPerc,yPerc);
  }
}

class btnVhs implements IMenuRunnable {
  String title() { return "VHS";}
  Boolean  saveHistory() { return true; }
  Boolean savePreFxImage() { return true; }
  String group(){ return "macro";}
  void run(int rowidx, int btnidx, int xPerc, int yPerc) {
    int[] menpos = getMenuPos();

//if ( img1.width < 700 || img1.height < 700 )
//RunRunnable("buffer", new int[] { 3, 5, 23, 64, 365, 438, 84, 112 });






/*
RunRunnable("fltInvert", new int[] { 3, 9, 40, 50, 1233, 1370, 84, 112 });

RunRunnable("echo1", new int[] { 2, 4, 47, 53, 548, 685, 56, 84 });

RunRunnable("blendSauce", new int[] { 1, 1, 39, 57, 137, 274, 28, 56 });

RunRunnable("fltInvert", new int[] { 3, 9, 29, 71, 1233, 1370, 84, 112 });

RunRunnable("brighter", new int[] { 5, 6, 23, 39, 822, 959, 140, 168 });

RunRunnable("contrast", new int[] { 5, 5, 60, 60, 685, 822, 140, 168 });

RunRunnable("contrast", new int[] { 5, 5, 60, 60, 685, 822, 140, 168 });

RunRunnable("contrast", new int[] { 5, 5, 60, 60, 685, 822, 140, 168 });

RunRunnable("fltInvert", new int[] { 3, 9, 29, 71, 1233, 1370, 84, 112 });


*/
RunRunnable("dubsiz", new int[] { 2, 8, 19, 57, 1096, 1233, 56, 84 });

RunRunnable("terminator", new int[] { 6, 8, 16, 32, 584, 657, 168, 196 });

RunRunnable("terminator", new int[] { 6, 8, 16, 32, 584, 657, 168, 196 });

RunRunnable("terminator", new int[] { 6, 8, 16, 32, 584, 657, 168, 196 });


RunRunnable("sinwav", new int[] { 6, 5, 86, 46, 365, 438, 168, 196 });

RunRunnable("sinwav", new int[] { 6, 5, 53, 42, 365, 438, 168, 196 });

RunRunnable("offset", new int[] { 4, 6, 93, 60, 438, 511, 112, 140 });

//RunRunnable("blendbuffer", new int[] { 4, 5, 20, 67, 365, 438, 112, 140 });

RunRunnable("blendmode", new int[] { 0, 1, 12, 57, 73, 146, 0, 28 });

RunRunnable("transbuffer", new int[] { 2, 7, 13, 57, 511, 584, 56, 84 });

RunRunnable("blendbuffer", new int[] { 4, 5, 34, 78, 365, 438, 112, 140 });

RunRunnable("blendbuffer", new int[] { 4, 5, 35, 78, 365, 438, 112, 140 });

RunRunnable("interlace", new int[] { 3, 4, 82, 57, 292, 365, 84, 112 });

RunRunnable("fltDilate", new int[] { 1, 12, 68, 82, 876, 949, 28, 56 });

RunRunnable("warp", new int[] { 4, 3, 54, 28, 219, 292, 112, 140 });

RunRunnable("offset", new int[] { 4, 6, 86, 60, 438, 511, 112, 140 });

RunRunnable("offset", new int[] { 4, 6, 82, 60, 438, 511, 112, 140 });

RunRunnable("offset", new int[] { 4, 6, 82, 60, 438, 511, 112, 140 });

RunRunnable("contrast", new int[] { 5, 5, 54, 71, 365, 438, 140, 168 });


/*
//vertblitz:
RunRunnable("edge000", new int[] { 3, 2, 16, 57, 274, 411, 84, 112 });

RunRunnable("vertdu", new int[] { 5, 2, 2, 85, 274, 411, 140, 168 });

RunRunnable("contrast", new int[] { 5, 5, 54, 57, 685, 822, 140, 168 });

RunRunnable("brighter", new int[] { 5, 6, 26, 53, 822, 959, 140, 168 });

RunRunnable("contrast", new int[] { 5, 5, 46, 50, 685, 822, 140, 168 });

RunRunnable("brighter", new int[] { 5, 6, 89, 53, 822, 959, 140, 168 });

*/


    
    
   // new fx chain: fake-scanlines+overlay+offset+sin-drift+reinsertio+darkmagic = __VHS__ smile emoticon
   // img1 = dus(img1,xPerc,yPerc);
  // btnInterlace()
  }
}

class btnTerminator implements IMenuRunnable {
  String title() { return "terminator";}
  Boolean  saveHistory() { return true; }
  Boolean savePreFxImage() { return true; }
  String group(){ return "system";}
  void run(int rowidx, int btnidx, int xPerc, int yPerc) {
    img1 = terminator(img1,xPerc,yPerc);
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
class btnHoriz3 implements IMenuRunnable {
  String title() { return "horiz3";}
  Boolean  saveHistory() { return true; }
  Boolean savePreFxImage() { return true; }
  String group(){ return "system";}
  void run(int rowidx, int btnidx, int xPerc, int yPerc) {
    img1 = dus(img1,xPerc,yPerc);
  }
}


PImage horiz3(PImage img, int xp, int yp) { 
 img.loadPixels();
 int diff = (int)map(xp,0,100,1,60);
 color pc = color(0), pr = color(0), pg = color(0), pb = color(0);
 for ( int i = img.pixels.length-1, l = img.pixels.length; i>=0; i--) { 
   int pos = i;
   color c = img.pixels[pos];
    if ( abs(brightness(c)-brightness(pc)) > diff )
      pc = c;
    if ( abs(red(c)-red(pr)) > diff )
      pr = c;
    if ( abs(green(c)-green(pg)) > diff )
      pg = c;
    if ( abs(blue(c)-blue(pr)) > diff )
      pb = c;
    img.pixels[pos] = color(red(pr),green(pg),blue(pb));
 }
 img.updatePixels();
 return img; 
}


class btnHiBufCol implements IMenuRunnable {
  String title() { return "hibufcol";}
  Boolean  saveHistory() { return true; }
  Boolean savePreFxImage() { return true; }
  String group(){ return "system";}
  void run(int rowidx, int btnidx, int xPerc, int yPerc) {
    int bs = buffer.size();
    if ( bs > 0 ) { 
      Log("hibuf on bfr "+(bs-1));
      PImage bfr = buffer.get(bs-1);
      img1 = mergeHiCol(img1,bfr,xPerc,50); // pass in hardcoded 50 to enfore no reverse and no sort
    } else {
     Log("buffer is empty"); 
    }
  }
}



class btnDus implements IMenuRunnable {
  String title() { return "dus";}
  Boolean  saveHistory() { return true; }
  Boolean savePreFxImage() { return true; }
  String group(){ return "system";}
  void run(int rowidx, int btnidx, int xPerc, int yPerc) {
    img1 = dus(img1,xPerc,yPerc);
  }

}

class btnBlendImageBuffer implements IMenuRunnable {
  String title() { return "blendbfr";}
  Boolean  saveHistory() { return true; }
  Boolean savePreFxImage() { return true; }
  String group(){ return "system";}
  void run(int rowidx, int btnidx, int xPerc, int yPerc) {
    int l = buffer.size();
    if ( l < 1 ) { 
      Log("no buffer");
      return;
    } 
    PImage last = buffer.get(l-1);
    last.loadPixels();
    last.pixels = reverse(last.pixels);
    last.updatePixels();
    PImage bmap = buffer.get(0);
    bmap.filter(POSTERIZE,3);
    bmap.filter(INVERT);
    img1 = blendImage(img1,last,bmap);
   // img1 = dus(img1,xPerc,yPerc);
  }
}

PImage blendImage(PImage img, PImage simg, PImage mimg) {
  img.loadPixels();
  simg.loadPixels();
  mimg.loadPixels();
  for ( int y = 0, h = img.height; y<h; y++) { 
    for ( int x = 0, w = img.width; x<w; x++) { 
      int sw = simg.width, mw = mimg.width, sh = simg.height, mh = mimg.height;
      int sx = (int)map(x,0,w,0,sw);
      int mx = (int)map(x,0,w,0,mw);
      int sy = (int)map(y,0,h,0,sh);
      int my = (int)map(y,0,h,0,mh);
      int pos = x + y * w;
      int spos = sx + sy * sw;
      int mpos = mx + my * mw;
      color c = img.pixels[pos];
      color sc = simg.pixels[spos];
      color mc = mimg.pixels[mpos];
      int opaq = (int)brightness(mc);
      c = color(red(c),green(c),blue(c),(opaq));
      sc = color(red(sc),green(sc),blue(sc),255-opaq);
      img.pixels[pos] = blendColor(c,sc,BLEND);
    }
  }
  img.updatePixels();
  return img; 
}



  PImage dus(PImage img, int xp, int yp) { 
    Log("dus");
    PImage mapImg = createImage(img.width,img.height,RGB);
    mapImg.copy(img,0,0,img.width,img.height,0,0,mapImg.width,mapImg.height);
    return mergeHiCol(img,mapImg,xp,yp);
  }
  
  
  PImage mergeHiCol(PImage img, PImage mapImg, int xp,int yp) { 
    PImage result = createImage(img.width,img.height,RGB);
    img.loadPixels();
    result.loadPixels();
    int maxc = max(img.pixels);
    int minc = min(img.pixels);
    mapImg.loadPixels();
    if ( yp <= 33 ) 
      mapImg.pixels = reverse(mapImg.pixels);
    else if (yp > 66 )
      mapImg.pixels = sort(mapImg.pixels);  
    mapImg.updatePixels();
    int thresh = (int)map(xp,0,100,minc,maxc);
    int ml = mapImg.pixels.length;
    for ( int i = 0, l = img.pixels.length; i<l; i++) {
      int mpos = (int)map(i,0,l,0,ml);
      color c = img.pixels[i];
      //println("c "+c+"< thresh "+thresh+" = "+( c < thresh )+""); 
      if ( c < thresh ) { 
         result.pixels[i] = img.pixels[i]; 
      } else { 
        result.pixels[i] = mapImg.pixels[mpos];
      }
    }
    result.updatePixels();
    return result;
  }




class btnSwitchColor implements IMenuRunnable {
  String title() { return "switchcolor";}
  Boolean  saveHistory() { return true; }
  Boolean savePreFxImage() { return true; }
  String group(){ return "system";}
  void run(int rowidx, int btnidx, int xPerc, int yPerc) {
    img1 = switchcolor(img1,xPerc,yPerc);
  }
}

class btnSpielerei implements IMenuRunnable {
  String title() { return "spielerei";}
  Boolean  saveHistory() { return true; }
  Boolean savePreFxImage() { return true; }
  String group(){ return "glitch";}
  void run(int rowidx, int btnidx, int xPerc, int yPerc) {
    img1 = spielerei(img1,xPerc,yPerc);
  }
}


class btnEdged implements IMenuRunnable {
  String title() { return "edged";}
  Boolean  saveHistory() { return true; }
  Boolean savePreFxImage() { return true; }
  String group(){ return "glitch";}
  void run(int rowidx, int btnidx, int xPerc, int yPerc) {
    ArrayList<PVector> points = edgeDetect(img1,(int)map(xPerc,0,100,1,80));
    Log("I have "+points.size()+" points");
    if ( yPerc > 50 ) {
      PImage tmpImg = createImage(img1.width,img1.height,RGB);
      img1= drawPoints(tmpImg,points,color(255)); 
    }else{
      img1= drawPoints(img1,points,color(255));
    }
  }
}


class btnDiag implements IMenuRunnable {
  String title() { return "diag";}
  Boolean  saveHistory() { return true; }
  Boolean savePreFxImage() { return true; }
  String logMsg = null;
  String group(){ return "glitch";}
  void run(int rowidx, int btnidx, int xPerc, int yPerc){
    img1 = diag(img1,xPerc,yPerc);
  }
}

class btnJar implements IMenuRunnable {
  String title() { return "jarimg";}
  Boolean  saveHistory() { return true; }
  Boolean savePreFxImage() { return true; }
  String logMsg = null;
  String group(){ return "glitch";}
  void run(int rowidx, int btnidx, int xPerc, int yPerc){
    img1 = jarimg(img1,xPerc,yPerc);
  }
}
class btnSlicer implements IMenuRunnable {
  String title() { return "slicer";}
  Boolean  saveHistory() { return true; }
  Boolean savePreFxImage() { return true; }
  String logMsg = null;
  String group(){ return "glitch";}
  void run(int rowidx, int btnidx, int xPerc, int yPerc){
    img1 = slicer(img1,yPerc,xPerc);
  }
}


class btnBg implements IMenuRunnable {
  String title() { return "diag";}
  Boolean  saveHistory() { return false; }
  Boolean savePreFxImage() { return false; }
  color[] bgcolors = { color(0),color(255),color(r1,r2,r3),color(0),color(r1,r1,r1),color(255),color(r2,r2,r2),color(0),color(r3,r3,r3),color(255),color(r3,r2,r1),color(0),color(r2,r3,r1),color(255),color(r2,r1,r3),color(0),color(r1,r3,r2),color(255),color(r3,r1,r2),color(0),color(255) };
  int coloridx = -1;
  String logMsg = null;
  String group(){ return "system";}
  void run(int rowidx, int btnidx, int xPerc, int yPerc){
    coloridx = coloridx < bgcolors.length -1 ? coloridx+1 : 0;
    bgColor = bgcolors[coloridx];
    //img1 = diag(img1,xPerc,yPerc);
  }
}

class btnSinsin implements IMenuRunnable {
  String title() { return "sinsin";}
  Boolean  saveHistory() { return true; }
  Boolean savePreFxImage() { return true; }
  String group(){ return "glitch";}
  String logMsg = null;
  int mod = 0;
  btnSinsin(){}
  btnSinsin(int i){
     this.mod = i;
  }
  void run(int rowidx, int btnidx, int xPerc, int yPerc){
    Log("Running in mode "+this.mod);
    switch (this.mod) { 
      case 0:
        img1 = sinsin(img1,xPerc,yPerc);
      break;
      case 1:
        img1 = sinsin2(img1,xPerc,yPerc);
      break;
      default:
        img1 = sinsin3(img1,xPerc,yPerc);
      break;
    }
  }
}

class btnStretch implements IMenuRunnable {
  String title() { return "stretch";}
  Boolean  saveHistory() { return true; }
  Boolean savePreFxImage() { return true; }
  String group(){ return "glitch";}
  String logMsg = null;
  int mod = 0;
  btnStretch(int i) { 
    this.mod = i;
  }
  void run(int rowidx, int btnidx, int xPerc, int yPerc){
    if ( this.mod == 0 ) { 
      img1 = stretch(img1,xPerc,yPerc);
    } else if ( this.mod == 1 ) { 
      img1 = stretch2(img1,xPerc,yPerc);
    } else {
      img1 = stretch3(img1,xPerc,yPerc);      
    }
  }
}

class btnFilter implements IMenuRunnable {
  String title() { return "filter";}
  Boolean  saveHistory() { return true; }
  Boolean savePreFxImage() { return true; }
  String group(){ return "glitch";}
  String logMsg = null;
  int filtr = 0;
  btnFilter() { }
  btnFilter(int fil) {
    this.filtr = fil;
  }
  void run(int rowidx, int btnidx, int xPerc, int yPerc){
    if ( this.filtr == 0 ) { 
    img1 = filtr(img1,xPerc,yPerc);
    } else { 
    img1 = flt(img1,this.filtr,xPerc,yPerc);
    }
  }
}

class btnPoster implements IMenuRunnable {
  String title() { return "poster";}
  Boolean  saveHistory() { return true; }
  Boolean savePreFxImage() { return true; }
  String logMsg = null;
  String group(){ return "glitch";}
  void run(int rowidx, int btnidx, int xPerc, int yPerc){
    img1 = posterize(img1,xPerc,yPerc);
  }
}

class btnContrast implements IMenuRunnable {
  String title() { return "bw";}
  Boolean  saveHistory() { return true; }
  Boolean savePreFxImage() { return true; }
  String logMsg = null;
    String group(){ return "glitch";}

  void run(int rowidx, int btnidx, int xPerc, int yPerc){
    img1.blend(img1,0,0,img1.width,img1.height,0,0,img1.width,img1.height,SCREEN);
   img1.blend(img1,0,0,img1.width,img1.height,0,0,img1.width,img1.height,MULTIPLY);
  }
}
class btnFaces implements IMenuRunnable {
  String title() { return "faces";}
  Boolean  saveHistory() { return true; }
  Boolean savePreFxImage() { return true; }
  String logMsg = null;
  String group(){ return "glitch";}
  void run(int rowidx, int btnidx, int xPerc, int yPerc){
    img1 = faces(_img1,img1,xPerc);
  }
}

class btnBrighter implements IMenuRunnable {
  String title() { return "brighter";}
  Boolean  saveHistory() { return true; }
  Boolean savePreFxImage() { return true; }
  String logMsg = null;
  String group(){ return "glitch";}
  void run(int rowidx, int btnidx, int xPerc, int yPerc){
    img1 = brighter(img1,xPerc,yPerc);
  }
}

//superLameGlitch(PImage img)

class btnSuperLame implements IMenuRunnable {
  String title() { return "superlame";}
  Boolean  saveHistory() { return true; }
  Boolean savePreFxImage() { return true; }
  String logMsg = null;
  String group(){ return "glitch";}
  void run(int rowidx, int btnidx, int xPerc, int yPerc){
    img1 = superLameGlitch(img1,xPerc,yPerc);
  }
}
class btnBw implements IMenuRunnable {
  String title() { return "bw";}
  Boolean  saveHistory() { return true; }
  Boolean savePreFxImage() { return true; }
  String logMsg = null;
  String group(){ return "glitch";}
  void run(int rowidx, int btnidx, int xPerc, int yPerc){
    img1 = bw(img1);
  }
}
class btnOffset implements IMenuRunnable {
  String title() { return "offset";}
  Boolean  saveHistory() { return true; }
  Boolean savePreFxImage() { return true; }
  String logMsg = null;
  String group(){ return "glitch";}
  void run(int rowidx, int btnidx, int xPerc, int yPerc){
    img1 = offsetImage(img1,xPerc,yPerc);
  }
}

class btnRgbOffset implements IMenuRunnable {
  String title() { return "rgboffset";}
  Boolean  saveHistory() { return true; }
  Boolean savePreFxImage() { return true; }
  String logMsg = null;
  String group(){ return "glitch";}
  void run(int rowidx, int btnidx, int xPerc, int yPerc){
    int mod = (int)map(yPerc,0,100,0,3);
    img1 = rgbOffsetImage(img1,xPerc,yPerc,mod);
  }
}






class btnWarp implements IMenuRunnable {
  String title() { return "warp";}
  Boolean  saveHistory() { return true; }
  Boolean savePreFxImage() { return true; }
  String logMsg = null;
  String group(){ return "glitch";}
  void run(int rowidx, int btnidx, int xPerc, int yPerc){
    img1 = warpImage(img1,xPerc,yPerc);
  }
}

class btnOpen implements IMenuRunnable {
  String title() { return "interlace";}
  Boolean  saveHistory() { return true; }
  Boolean savePreFxImage() { return true; }
  String logMsg = null;
  String group(){ return "system";}
  void run(int rowidx, int btnidx, int xPerc, int yPerc){
    loadFile();
  }
}



class btnBuffer implements IMenuRunnable { 
  String title() { return "buffer"; }
  Boolean saveHistory() { return false; }
  Boolean savePreFxImage() { return false; }
  String group(){ return "buffer";}
  btnBuffer() { 
    buffer = new ArrayList<PImage>();
  } 
  void run(int rowidx, int btnidx, int xPerc, int yPerc){
    if ( xPerc < 50 ) { 
      // push buffer
      PImage toPush = createImage(img1.width,img1.height,RGB);
      toPush.copy(img1,0,0,img1.width,img1.height,0,0,toPush.width,toPush.height);
      buffer.add(toPush);
      bimg1 = toPush;
      Log("pushed buffer");
    } else { 
      if ( buffer.size() > 0 ) { 
        bimg1 = buffer.get(buffer.size()-1);
        buffer.remove(buffer.size()-1); 
        Log("popped buffer "+buffer.size());
      } else { 
         Log("buffer is empty "+buffer.size()); 
      }
    }
  }
}

class btnBlendBuffer implements IMenuRunnable {
  String title() { return "blendbuffer";}
  Boolean  saveHistory() { return true; }
  Boolean savePreFxImage() { return false; }
  String group(){ return "buffer";}
  void run(int rowidx, int btnidx, int xPerc, int yPerc) {
    if ( bimg1 != null ) { 
      Log("Runner: Blending buffer with "+blendTitles[blendIdx]);
          img1.blend(bimg1,0,0,bimg1.width,bimg1.height,0,0,img1.width,img1.height,blendMode);
    } else { 
      Log("buffer is empty");
    }
  }
}

// vertdu(PImage img, int xp, int yp)

class btnVertdu implements IMenuRunnable {
  String title() { return "vertdu";}
  Boolean saveHistory() { return true; }
  Boolean savePreFxImage() { return true; }
  String logMsg = null;
    String group(){ return "glitch";}

  void run(int rowidx, int btnidx, int xPerc, int yPerc){
    img1 = vertdu(img1,xPerc,yPerc);
  }
}
class btnHordu implements IMenuRunnable {
  String title() { return "hordu";}
  Boolean saveHistory() { return true; }
  Boolean savePreFxImage() { return true; }
  String logMsg = null;
  String group(){ return "glitch";}
  void run(int rowidx, int btnidx, int xPerc, int yPerc){
    img1 = hordu(img1,xPerc,yPerc);
  }
}
class btnMirror implements IMenuRunnable {
  String title() { return "mirror";}
  Boolean  saveHistory() { return true; }
  Boolean savePreFxImage() { return true; }
  String logMsg = null;
  String group(){ return "glitch";}
  void run(int rowidx, int btnidx, int xPerc, int yPerc){
    img1 = mirror(img1,xPerc,yPerc);
  }
}

class btnYMirror implements IMenuRunnable {
  String title() { return "ymirror";}
  Boolean  saveHistory() { return true; }
  Boolean savePreFxImage() { return true; }
  String logMsg = null;
  String group(){ return "glitch";}
  void run(int rowidx, int btnidx, int xPerc, int yPerc){
    img1 = ymirror(img1,xPerc,yPerc);
  }
}

class btnMode implements IMenuRunnable {
  String title() { return "mode";}
  Boolean  saveHistory() { return false; }
  Boolean savePreFxImage() { return false; }

  String group(){ return "system";}
  void run(int rowidx, int btnidx, int xPerc, int yPerc) { 
    xymode = 
      xymodeIdx < xymodes.length - 1 ? 
      (xymodeIdx+=1) : 
      (xymodeIdx=0);
      Log("MenuRunner Setting xymode to "+xymode+" ("+xymodes[xymodeIdx]+") ");
  }
}

class btnBlendMode implements IMenuRunnable {
   String title() { return "blendmode";}
  Boolean  saveHistory() { return false; }
  Boolean savePreFxImage() { return false; }
  String group(){ return "system";}

  void run(int rowidx, int btnidx, int xPerc, int yPerc) {
    
    blendIdx = (int)map(xPerc,0,100,0,blendModes.length);
    blendMode = blendModes[blendIdx];
    Log("Runner: Blendmode set to "+blendTitles[blendIdx]+"("+blendIdx+") for input at "+xPerc+"%.");
  }
}

class btnRevert implements IMenuRunnable {
  String title() { return "revert";}
  Boolean  saveHistory() { return true; }
  Boolean savePreFxImage() { return true; }
  String group(){ return "system";}

  void run(int rowidx, int btnidx, int xPerc, int yPerc) {
          if (_img1 != null) { 
            img1.copy(_img1,0,0,_img1.width,_img1.height,0,0,img1.width,img1.height);
            Log("MenuRunner Reverted");
            //redraw();
            Log("MenuRunner Barred redraw");
          }
  
  }
}

class btnReset implements IMenuRunnable {
  String title() { return "reset";}
  Boolean  saveHistory() { return false; }
  Boolean savePreFxImage() { return false; }
  String group(){ return "system";}
  void run(int rowidx, int btnidx, int xPerc, int yPerc) {
    resetAll();
    Log("Saved");      
  }
}


class btnSave implements IMenuRunnable {
  String title() { return "save";}
  Boolean  saveHistory() { return true; }
  Boolean savePreFxImage() { return true; }
  String group(){ return "system";}
  void run(int rowidx, int btnidx, int xPerc, int yPerc) {
          if ( img1 != null) saveImage(img1);

  }
}
class btnExit implements IMenuRunnable {
  String title() { return "exit";}
  Boolean  saveHistory() { return true; }
  Boolean savePreFxImage() { return true; }
  String group(){ return "system";}
  void run(int rowidx, int btnidx, int xPerc, int yPerc) {
    Log("Bye");
    exit();
  }
}



class btnSelfie implements IMenuRunnable {
  String title() { return "selfie";}
  Boolean  saveHistory() { return true; }
  Boolean savePreFxImage() { return true; }
  String group(){ return "glitch";}
  void run(int rowidx, int btnidx, int xPerc, int yPerc) {
    img1 = selfie(img1, xPerc, yPerc);
  }
}

class btnTransBuffer implements IMenuRunnable {
  String title() { return "transbuffer";}
  Boolean  saveHistory() { return true; }
  Boolean savePreFxImage() { return true; }
  String group(){ return "buffer";}
  void run(int rowidx, int btnidx, int xPerc, int yPerc) {
    if ( buffer == null || buffer.size() < 1 ) { 
      Log("TransBuffer reports empty buffer"); 
      return;
    }
    int l = buffer.size();
    buffer.set(l-1,trans(buffer.get(l-1),xPerc,yPerc));
    Log("Transbuffer set buffer opacity to "+xPerc+"%");
  }
}

class btnBrDrift implements IMenuRunnable {
  String title() { return "brDrift";}
  Boolean  saveHistory() { return true; }
  Boolean savePreFxImage() { return true; }
  String group(){ return "glitch";}
  void run(int rowidx, int btnidx, int xPerc, int yPerc) {
    img1 = brDrift(img1,xPerc,yPerc);
  }
}

class btnResize implements IMenuRunnable {
  String title() { return "resize";}
  Boolean  saveHistory() { return true; }
  Boolean savePreFxImage() { return true; }
  String group(){ return "effect";}
  float wratio = 1.0;
  float hratio = 1.0;
  btnResize(float w, float h) { 
    this.wratio = w;
    this.hratio = h; 
  }
  void run(int rowidx, int btnidx, int xPerc, int yPerc) {
    img1 = resizeimg(img1,(int)(img1.width*this.wratio), (int)(img1.height*this.hratio));
  }
}


class btnDoubleX implements IMenuRunnable {
  String title() { return "double";}
  Boolean  saveHistory() { return true; }
  Boolean savePreFxImage() { return true; }
  String group(){ return "system";}
  int mod = 0;
  btnDoubleX(int m) { 
    this.mod = m;
  }
  void run(int rowidx, int btnidx, int xPerc, int yPerc) {
    img1 = doubleimg(img1,xPerc,yPerc, this.mod);
  }
}



interface IMenuRunnable {
  String title();// = "iMenuRunnable";
  String group();
  Boolean saveHistory();// = false;
  Boolean savePreFxImage();// = false;
  void run(int rowidx, int btnidx, int xPerc, int yPerc);
}
// echoImage(PImage img, int amount, int mode)
class btnEcho1 implements IMenuRunnable {
  int mode = 0; 
  String title() { return "echo1"; }
  String group(){return "glitch";} 
  Boolean saveHistory() { return true; } 
  Boolean savePreFxImage() { return true; }
  btnEcho1() { }
  btnEcho1(int mode) { 
    this.mode = mode;   
  }
  void run(int rowidx, int btnidx, int xPerc, int yPerc) {
    int echopos = floor(img1.width*xPerc/100);
    img1 = echoImage(img1, echopos, mode);   
  }
}
class btnUndo implements IMenuRunnable {
  String title() { return "Undo";}
  Boolean  saveHistory() { return false; }
  Boolean savePreFxImage() { return false; }
  String group(){return "system";} 
  void run(int rowidx, int btnidx, int xPerc, int yPerc) {
    undo();
  }
}
class btnRedo implements IMenuRunnable {
  String title() { return "Redo";}
  Boolean  saveHistory() { return false; }
  Boolean savePreFxImage() { return false; }
  String group(){return "system";} 
  void run(int rowidx, int btnidx, int xPerc, int yPerc) {
          if ( historyIdx < history.size() ) {
            PImage pimg = history.get(historyIdx);
            historyIdx++;
            img1.copy(pimg,0,0,pimg.width,pimg.height,0,0,img1.width,img1.height);
          } 
  }
}

class btnResetPrev implements IMenuRunnable {
  String title() { return "Restore Prev";}
  Boolean  saveHistory() { return false; }
  Boolean savePreFxImage() { return false; }
  String group(){return "system";} 
  void run(int rowidx, int btnidx, int xPerc, int yPerc) {
        img1.copy(pimg1,0,0,pimg1.width,pimg1.height,0,0,img1.width,img1.height);  
  }
}

class btnPxWidth implements IMenuRunnable {
  String title() { return "Set PxWidth";}
  Boolean  saveHistory() { return false; }
  Boolean savePreFxImage() { return false; }
  String group(){return "system";} 
  void run(int rowidx, int btnidx, int xPerc, int yPerc) {
          Log("MenuControl Setting pxWidth to "+(++pxWidth));  
  }
}

class btnPxHeight implements IMenuRunnable {
  String title() { return "Set PxHeight";}
  Boolean  saveHistory() { return false; }
  Boolean savePreFxImage() { return false; }
  String group(){return "system";} 
  void run(int rowidx, int btnidx, int xPerc, int yPerc) {
          Log("MenuControl Setting pxHeight to "+(++pxHeight));
  }
}

class btnSensitivity implements IMenuRunnable {
  String title() { return "Set Sensitivity";}
  Boolean  saveHistory() { return false; }
  Boolean savePreFxImage() { return false; }
  String group(){return "system";} 
  void run(int rowidx, int btnidx, int xPerc, int yPerc) {
          Log("MenuControl Setting sensitivity to "+(++sensitivity));
  
  }
}
class btnLameSort implements IMenuRunnable {
  String title() { return "Lame Sort";}
  Boolean  saveHistory() { return true; }
  Boolean savePreFxImage() { return true; }
    String group(){return "glitch";} 

  void run(int rowidx, int btnidx, int xPerc, int yPerc) {
          Log("MenuControl Executing LameSort");
            img1 = lameSort(img1);  
  }
}

class btnBlendSauce implements IMenuRunnable {
  String title() { return "Blend Sauce";}
  Boolean  saveHistory() { return true; }
  Boolean savePreFxImage() { return true; }
  String group(){return "buffer";} 
  void run(int rowidx, int btnidx, int xPerc, int yPerc) {
      Log("Runner: Blending sauce with "+blendTitles[blendIdx]);
          img1.blend(_img1,0,0,_img1.width,_img1.height,0,0,img1.width,img1.height,blendMode);
          
  }
}
class btnBlendPrev implements IMenuRunnable {
  String title() { return "Blend Prev";}
  Boolean  saveHistory() { return true; }
  Boolean savePreFxImage() { return true; }
    String group(){return "buffer";} 

  void run(int rowidx, int btnidx, int xPerc, int yPerc) {

          if ( pimg1 != null ) {
            img1.blend(pimg1,0,0,_img1.width,_img1.height,0,0,img1.width,img1.height,blendMode);
          }  
  }
}
class btnLameGlitch implements IMenuRunnable {
  String title() { return "Lame Glitch";}
  Boolean  saveHistory() { return true; }
  Boolean savePreFxImage() { return true; }
    String group(){return "glitch";} 

  void run(int rowidx, int btnidx, int xPerc, int yPerc) {
          backupImage();
          img1 = lameGlitch(img1);
  }
}

class btnLameGlitch2 implements IMenuRunnable {
  String title() { return "lameglitch2";}
  Boolean  saveHistory() { return true; }
  Boolean savePreFxImage() { return true; }
    String group(){return "glitch";} 

  void run(int rowidx, int btnidx, int xPerc, int yPerc) {
          backupImage();
          img1 = lameglitch2(img1,xPerc,yPerc);
  }
}

class btnEdge002 implements IMenuRunnable {
  String title() { return "Edge 002";}
  Boolean  saveHistory() { return true; }
  Boolean savePreFxImage() { return true; }
    String group(){return "glitch";} 

  void run(int rowidx, int btnidx, int xPerc, int yPerc) {

          img1 = edge002(img1);  
  }
}


class btnEdge000 implements IMenuRunnable {
  String title() { return "Edge 000";}
  Boolean  saveHistory() { return true; }
  Boolean savePreFxImage() { return true; }
  String group(){return "glitch";} 

  void run(int rowidx, int btnidx, int xPerc, int yPerc) {

          img1 = edge000(img1);
          
  }
}


class btnVert implements IMenuRunnable {
  String title() { return "Vert";}
  Boolean  saveHistory() { return true; }
  Boolean savePreFxImage() { return true; }
  String group(){return "glitch";} 
  void run(int rowidx, int btnidx, int xPerc, int yPerc) {
          img1 = vert(img1);  
  }
}

class btnInterlace implements IMenuRunnable {
  String title() { return "Interlace";}
  Boolean  saveHistory() { return true; }
  Boolean savePreFxImage() { return true; }
  String group(){return "glitch";} 
  void run(int rowidx, int btnidx, int xPerc, int yPerc) {
          img1 = lameLace(img1);
          
  }
}

class btnVertBlur implements IMenuRunnable {
  String title() { return "Vertical Blur";}
  Boolean  saveHistory() { return true; }
  Boolean savePreFxImage() { return true; }
  String group(){return "glitch";} 
  void run(int rowidx, int btnidx, int xPerc, int yPerc) {
          img1 = vertBlurImage(img1,(int)map(xPerc,0,100,1,88));  
  }
}
//side effects may include saving of logfiles, macros, ... 
void saveImage(PImage img) {
  String latestPath = "c:\\dus.jpg";
  String archivePathBase = "c:\\archive\\"+year()+"\\"+month()+"\\";//"//+".png"; //fixme, sanitiza imgName
  String fileNam = +year()+"."+month()+"."+day()+"-"+hour()+"."+minute()+"."+second()+"."+imgName;
  String archivePath = archivePathBase +fileNam+".png";
  String archiveLogPath = archivePathBase + "log\\"+fileNam+".log.txt";
  img.save(archivePath);
  Log(archivePath);
  SaveLog(archiveLogPath);
  SaveMacro();
}
void Log(String txt) {
  Boolean debug = false;
  String logTxt = hour()+":"+minute()+" "+txt+"\n";
  print(logTxt);
  if ( !debug && ( txt.length() < 11 || !txt.substring(0,11).equals("RunRunnable") ) ) return;
  logBuffer.add(logTxt);
}
void SaveLog(String path) {
  if ( logBuffer.size() > 0 )
    saveStrings(path, logBuffer.toArray(new String[logBuffer.size()])); 
}
void SaveMacro() { 
  Boolean debug = false;
  if ( logBuffer.size() > 0 && !debug ) {
    String[] macro = logBuffer.toArray(new String[logBuffer.size()]);
    String className = "ma"+year()+month()+day()+hour()+minute()+second();
    String classy = "class "+className+" implements IGlitch {\n";
    for ( int i = 0, l = macro.length; i<l; i++ )  { 
      classy += macro[i] + "\n"; 
    }
    classy+="\n}\n";
    println(classy);
  }
}
void write(String txt, int x, int y, int ftSize, color fg, color bg){
  fill(bg);
  noStroke();
  int rowi = (int)((txt.length()*ftSize)/3*1.5);
  int bgypos = y-ftSize+(ftSize/4);
  rect(x,bgypos,rowi,ftSize);
  textSize(10);
  fill(fg);
  textFont(fnt1,ftSize);
  text(txt,x,y);
}
void drawLog() { 
  int ctr = 0;
  int fs = 18;
  int n = logBuffer.size();
  int maxLines = floor((height-menuHeight)/fs);
  maxLines = 4; // meh
  int top = fs-2+height-((n>maxLines?maxLines:n)*fs);
  //top = menuHeight+4*fs;
  int padding = 10;
  fill(bgLogColor);
  //rect(0,height-3*fs,width,3*fs);
  for ( int i = logBuffer.size()-1 >= maxLines ? (logBuffer.size() - maxLines) : 0; i<n; i++ ){
    write(logBuffer.get(i),10,top+ctr*fs,fs,fgLogColor,bgLogColor);
    ctr++;
  } 
}
void drawStats() { 
  int fs = 12; 
  String stats = "";
  if ( _img1 != null ) { 
    stats = "sauce:\nw: "+_img1.width+"px\nh: "+_img1.height+"\ncurrent:\nw: "+img1.width+"px\nh: "+img1.height+"px\n";//display:\nw: ";  
  }
  stats += "sys:\nlogsize: "+logBuffer.size()+"lines\nhistory: "+history.size()+"pcs\nblendmode: "+blendTitles[blendIdx];
  write(stats,10,menuHeight+fs,fs,fgLogColor,bgLogColor);
  
}
void addHistory() {
  Log("adding history");
  if ( historyIdx != history.size() ) { 
    for (int i = history.size()-1; i > historyIdx ; i--) { 
       history.remove(i);
    }
  }
  if ( img1 != null ) { 
    PImage himg = createImage(img1.width,img1.height,RGB);
    himg.copy(img1,0,0,img1.width,img1.height,0,0,himg.width,himg.height);
    history.add(himg);
    historyIdx = history.size();
  }
}
void backupImage() { 
    if( img1 != null ) 
    {
      if ( pimg1 == null ) pimg1 = img1;
      pimg1.copy(img1,0,0,img1.width,img1.height,0,0,img1.width,img1.height);
    }
}
int[] getMenuPos() {
      int btnNumber = (int)map(mouseX,0,width-2,0,buttonCount);
      int btnRowNumber = (int)map(mouseY,0,menuHeight-2,0,buttonRowCount);
      int btnLeftLimit = btnNumber*btnWidth;
      int btnRightLimit = (btnNumber*btnWidth)+btnWidth;
      int btnTopLimit = btnRowNumber * btnHeight;
      int btnBotLimit = (btnRowNumber*btnHeight)+btnHeight;
      int xPerc = (int)map(mouseX,btnLeftLimit,btnRightLimit,0,100);
      int yPerc = (int)map(mouseY,btnBotLimit,btnTopLimit,0,100);
  return new int[] { btnRowNumber, btnNumber, xPerc, yPerc, btnLeftLimit, btnRightLimit, btnTopLimit, btnBotLimit  };
}
void resetAll() { 
  Log("ResetAll Resetting history, logBuffer, userVariables and mainImg to initial state."); 
  history = new ArrayList<PImage>();
  logBuffer = new ArrayList<String>();
  historyIdx = 0;
  _img1 = pimg1 = img1 = null; 
}

void mouseClicked() { 
  imgY = mouseY - menuHeight;
  imgX = mouseX - leftMargin;
  String btn = mouseButton == LEFT ? "left" : ( mouseButton == RIGHT ? "right" : "other" );
  Log("MouseClick ("+btn+") at image location: "+imgX+"x"+imgY+" on x: "+mouseX+", y:"+mouseY);
  if ( mouseButton == LEFT ) { 
  if ( mouseY < menuHeight ) { 
      //topbar menu
      int[] menpos = getMenuPos();
      int btnRowNumber = menpos[0];// (int)map(mouseY,0,menuHeight-2,0,buttonRowCount);
      int btnNumber = menpos[1];//(int)map(mouseX,0,width-2,0,buttonCount);
      int xPerc = xPercent = menpos[2];// (int)map(mouseX,btnLeftLimit,btnRightLimit,0,100);
      int yPerc = yPercent = menpos[3];// (int)map(mouseY,btnBotLimit,btnTopLimit,0,100);
      int btnLeftLimit = menpos[4];// btnNumber*btnWidth;
      int btnRightLimit = menpos[5];// (btnNumber*btnWidth)+btnWidth;
      int btnTopLimit = menpos[6];// btnRowNumber * btnHeight;
      int btnBotLimit = menpos[7];// (btnRowNumber*btnHeight)+btnHeight;
      //println("perc"+xPerc+" is "+mouseX+" between "+ btnLeftLimit + " and " + btnRightLimit +" as 0:100");
      //int yPerc = 0;
      Log("MenuControl ["+buttonText[btnRowNumber][btnNumber]+"] Btn: "+btnNumber+", Row: "+btnRowNumber+" :"+" xPerc: "+xPerc+"%, yPerc: "+yPerc+"% ");

      String selectedButton = buttonText[btnRowNumber][btnNumber];
      RunRunnable(selectedButton,menpos);    
/*      
      IMenuRunnable runner;
      if ( ( runner = menuItems.get(selectedButton))!=null) {
        //Log("All-RiGHT L:)) New MenuItem "+runner.title());
        if ( runner.savePreFxImage() ) backupImage();
        runner.run(menpos[0],menpos[1],menpos[2],menpos[3]);
        redraw();
        //if ( runner.logMsg != null ) Log(runner.logMsg);
        if ( runner.saveHistory() ) addHistory();
      } else { 
        Log("PLEASE IMPLEMENT BUTTON: "+buttonText[btnRowNumber][btnNumber]+"");
        //topClick(btnRowNumber,btnNumber,xPerc,yPerc);
      }
*/

  } else { 
    if ( imgX >= 0 && imgX < img1.width  && imgY >= 0 && imgY < img1.height ) {
      handleXYClick(imgX,imgY);
    }
    // bottom xypad
  }
  } else {
   Log("RIGHTCLICK"); 
     undo(); 
     redraw();
    
  }
}
void setWait(Boolean toggle) { 
        $("body").css("cursor", toggle ? "progress" : "auto");
}

void RunRunnable(String runtitle, int[] menpos) { 
setWait(true);
      IMenuRunnable runner;
      Log("RunRunnable(\""+runtitle+"\", new int[] { "+intString(menpos)+" });");
      if ( ( runner = menuItems.get(runtitle))!=null) {
        //Log("All-RiGHT L:)) New MenuItem "+runner.title());
        if ( runner.savePreFxImage() ) backupImage();
        runner.run(menpos[0],menpos[1],menpos[2],menpos[3]);
        redraw();
        //if ( runner.logMsg != null ) Log(runner.logMsg);
        if ( runner.saveHistory() ) addHistory();
      } else { 
        Log("PLEASE IMPLEMENT BUTTON: "+runtitle+"");
        //topClick(btnRowNumber,btnNumber,xPerc,yPerc);
      }
 setWait(false);
}

String intString(int[] ints) { 
  String result = "";
  for ( int i =0, l=ints.length; i<l; i++ ) {
    result += String.valueOf(ints[i]);
    if ( i != l-1 ) result += ", ";
  }
  return result;
}
void handleXYClick(int ix, int iy) {
    int  iw = img1.width, ih = img1.height; 
    float varx = map(ix,0,iw,0,100);
    float vary = map(iy,0,ih,0,100);
    int ip = ix+iy*iw;
    switch(xymode) { 
      case 0: //command
      break;
     case 1: //insert
       img1 = lameInsert(img1,ix,iy);
       redraw();
     break;
    }
}

void loadTextFile() { 
  println("input selecting");
  selectInput("Macro!","loadTextFileCallback");
  println("input selected");
}
void loadTextFileCallback(File selectedFile) {
println("dus");
  if ( selectedFile != null ) { 
println("wtf");
    String textPath = selectedFile.getAbsolutePath();
    println("SELECTED: "+textPath);
    String[] macroLines = loadStrings(textPath);
    if ( macroLines != null && macroLines.length > 0 ) { 
      handleMacro(macroLines);
    }   
 }
}



void handleMacro(String[] lines) { 
  for ( int i = 0, l = lines.length; i<l; i++ ) { 
     parseMacroLine(lines[i]); 
    // save("c:\\macro_"+i+".png");
  }
}


void parseMacroLine(String line) { 
  // check for RunRunnable:
  if ( line.length() > 20 ) { 
    println("]"+line.substring(6,17)+"[");
    if ( line.substring(6,17).equals("RunRunnable") ) { 
      //ok, standard v0.1 macro line :) let's try to parse and execute it :)
      String command = line.substring(line.indexOf("\"")+1);
      command = command.substring(0,command.indexOf("\""));
      String args = line.substring(line.indexOf("{")+1,line.indexOf("}"));
      String[] argpart = split(args, ',');
      int[] macroArgs = new int[argpart.length];
      for ( int i = 0, l = argpart.length; i < l; i++ ) { 
         macroArgs[i] = Integer.parseInt(argpart[i].trim());
      }
      if ( command != null && !command.equals("")) { 
        // blacklist some commands
        if ( command.equals("open") ) return;
        if ( command.equals("save") ) return;
        if ( command.equals("macroo") ) return;
        RunRunnable(command,macroArgs); 
        
      }
    }
  }  
}




//String userHome = System.getProperty("user.home");
void loadFile() { 
//  selectInput("File!","loadFileCallback"); 
    $('#filer').click();
}
void loadFileCallback(File selected) { 
  if ( selected != null ) { 
    imgPath = selected.getAbsolutePath();
    imgName = selected.getName();
    img1 = loadImage(selected.getAbsolutePath());
    Log("Opening '"+imgName+"' from '"+imgPath+"'"); 
    fileIsLoaded();
  }
}

void fileIsLoaded() { 
  int sanityCounter = 1;
  int maxSanityChecks = 20;
  while (sanityCounter < maxSanityChecks ) { 
    try { 
    _img1 = createImage(img1.width,img1.height,RGB);
    _img1.copy(img1,0,0,img1.width,img1.height,0,0,img1.width,img1.height);//loadImage(selected.getAbsolutePath()); //FIXME, copy by value how?
    Log("ImageLoader image loaded");
     sanityCounter=maxSanityChecks;
     fileSelected();
     return;
    } catch(Error e) { } 
    // fixme: sleep
    Log("ImageLoader failed to load image at attempt #"+sanityCounter);
    sanityCounter++;
  }
}




void fileSelected() {
  img1.loadPixels(); 
  imgW = img1.width;
  imgH = img1.height;
  if ( imgW < frameWidth ) { 
    leftMargin = (frameWidth - imgW)/2; 
  } else { 
    leftMargin = 0;
    size(imgW,menuHeight+imgH);
  }
  addHistory();
  redraw();
}

void drawMenu2() { 
  return;/*
  if ( menuItems == null || menuItems.size() < 1 ) { 
     Log("ARG, no MenuItems!!");
     return;
  } 
  int maxperrow = 10;
  int btns = maxperrow;
  int l = menuItems.size();
  int rows = (int)(l/maxperrow);
  if ( l < maxperrow ) btns = l;
  if ( l % maxperrow != 0 ) rows += 1;
  int btnW = 0, btnH = 0, btnX = 0, btnY = 0;
  btnW = (int)((width-2)/btns);
  btnH = (int)((menuHeight-2)/rows );
  int menuPadding = ( width - ( btns * btnW ))/2;
  int fntHeight = (int)(btnH/phi);
  int textTopPadding = 2+(int)((btnH - fntHeight)*phi);
  int textPadding = 10;
  stroke(color(0,255,0));
  strokeWeight(1);
  int idx = 0;
  for ( int by = 0; by < rows; by++ ) { 
    for (int bx = 0; bx < btns; bx++ ) { 
        rect(bx*btnW,by*btnH,btnW,btnH);
      stroke(fgMenuColor);
      fill(bgMenuColor);
      rect(menuPadding+(bx*btnW),(by*btnH),btnW,btnH);
      //stroke(color(red(fgMenuColor),green(fgMenuColor),blue(fgMenuColor),35));
      rect(menuPadding+(bx*btnW)+btnW/4,(by*btnH)+btnH/4,btnW/4,btnH/2);
      rect(menuPadding+(bx*btnW)+btnW/4*2,(by*btnH)+btnH/4,btnW/4,btnH/2);
       stroke(fgMenuColor);
    }   
  }*/
}
void drawMenu() { 
  btnWidth = (int)((width-2)/buttonCount);
  btnHeight = (int)((menuHeight-2)/buttonRowCount );
  int menuPadding = ( width - ( buttonCount * btnWidth ))/2;
  int fntHeight = (int)constrain(btnHeight/phi,2,14);
  int textTopPadding = (int)fntHeight;//((btnHeight/2 - fntHeight)*phi);
  int textPadding = 10;
  int rows = img1==null?1:buttonRowCount;
  for (int r = 0; r<rows; r++) { 
    for (int i = 0; i<buttonCount; i++) {
      stroke(fgMenuColor);
      fill(bgMenuColor);
      rect(menuPadding+(i*btnWidth),(r*btnHeight),btnWidth,btnHeight);
      stroke(color(red(fgMenuColor),green(fgMenuColor),blue(fgMenuColor),35));
      rect(menuPadding+(i*btnWidth)+btnWidth/4,(r*btnHeight)+btnHeight/4,btnWidth/4,btnHeight/2);
      rect(menuPadding+(i*btnWidth)+btnWidth/4*2,(r*btnHeight)+btnHeight/4,btnWidth/4,btnHeight/2);
       stroke(fgMenuColor);
     
      if ( buttonText[r][i] != null ) {
        textFont(fnt1,fntHeight);
        noStroke();
        fill(fgMenuColor);
        text(buttonText[r][i], textPadding+menuPadding+(i*btnWidth), textTopPadding+(r*btnHeight) );
      }
    }
  }
}

void drawImage(PImage img) { 
  PImage displayImage;
  if ( img.width > img.height ) { 
      if (img.width < frameWidth ) { 
        println("1");
        displayImage = createImage(img.width,img.height,RGB);
      } else { 
        println("2");
        int w = 800,h = 600;
        // img = 15x10 
        // maxw = 10;
        // 15/10 = 10 / x;
        // 
        /*
800      400
---- == ----- 
600       x

x * ( 800 / 600 ) = 400
x * ( 800 ) = 400 *  600
x = 400 * 600 / 800

800/800 == 400 / x 
-------
600/800

h=maxw/h/w;



     i love my poepie :)    
        
        */
        h = w * img.height/img.width;
        displayImage = createImage(w,h,RGB);
      }
  } else { 
      if ( img.height < 500 ) { 
                println("3");
        displayImage = createImage(img.width,img.height,RGB);        
      } else { 
                println("4");
        int w = 450,h = 600;
        w = h * img.width/img.height;
        displayImage = createImage(w,h,RGB); 
      }
  }
  displayImage.copy(img,0,0,img.width,img.height,0,0,displayImage.width,displayImage.height);
//  leftMargin = img.width < width ? (width - img.width)/2 : 0; 
  leftMargin = displayImage.width < width ? (width - displayImage.width)/2 : 0; 
  
  //draw border around image:  
  //stroke(fgMenuColor);
  //noFill();
  //rect(leftMargin-10,menuHeight + 50, displayImage.width + 20, displayImage.height + 20);
  
  image(displayImage,leftMargin,menuHeight+60); 
}




/* !!glitch bitch!! */

PImage lameGlitch(PImage img) { 
  img.loadPixels();
  int l = img.pixels.length;
  if ( img.pixels.length < 200 ) { 
    Log("too small an image");//set the 200 to actual number from below
    return null;
  }
  int p = l/3*2; // 2/3ds into the array
  int g = 20000;
  int[] bfr = new int[g];
  int grabPos = (int)(random(1)*(img.pixels.length-g));
  arrayCopy(img.pixels,grabPos,bfr,0,g);
  for ( int i = 0; i < img.pixels.length-g; i+= g*(1+(int)(random(1)*10))) { 
    arrayCopy(bfr,0,img.pixels,i,g);
  }
 // arrayCopy(img.pixels,p+g,img.pixels,p,img.pixels.length-p+g-1);
 // arrayCopy(bfr,0,img.pixels,img.pixels.length-g,g-1);
  img.updatePixels();
  return img;
}
/*

length = 10;
bfrlength = 3;
glpos = 4;
bfr[bfrlength] = 0,1,2;

ac(src,0,bfr,bfrlength

*/
int maxRecurse = 20;

PImage lameInsert(PImage img, int ix, int iy) {
  int mindiff = sensitivity;
  fill(255);
  rect(0,0,400,300);
  img.loadPixels();
  int w = img.width, h = img.height;
  int p = ix + iy * w;
  //find toplimit
  color pc,tc;
  int tl = 1, ll =1, bl = 1, rl = 1;
  if ( p > w * 2 ) { 
    pc = img.pixels[p];
    int np = p-w;
    tc = img.pixels[np];
    Log("diff: "+abs(brightness(tc)-brightness(pc)));
    while (abs(brightness(tc)-brightness(pc)) < mindiff && np >= w) { 
      tl++;
      tc = img.pixels[np];
      np-=w;
    }
    np = p+w;
    tc = img.pixels[np];
    while (abs(brightness(tc)-brightness(pc)) < mindiff && np >= w) { 
      bl++;
      np+=w;
      tc = img.pixels[np];
    }
    np = p+1;
    tc = img.pixels[np];
    while (abs(brightness(tc)-brightness(pc)) < mindiff && np >= w) { 
      rl++;
      np+=1;
      tc = img.pixels[np];
    }
    np = p-1;
    tc = img.pixels[np];
    while (abs(brightness(tc)-brightness(pc)) < mindiff && np >= w) { 
      ll++;
      np-=1;
      tc = img.pixels[np];
    }
    int ox = ix - ll;
    int oy = iy - tl;
    int ow = ix + rl - ox;
    int oh = iy + bl - oy;
    for (int yy = oy; yy < oy+oh; yy++ ) {
      
    for (int xx = ox; xx < ox+ow; xx++ ) {
        int sqpos = xx + yy * img.width;
        img.pixels[sqpos] = pc;
    } 
    }   
    Log("toplimit:" + tl + "rect("+ix+","+(iy-tl)+",3,"+tl+","+tc+")");
    imgRect(ix,iy-tl,3,tl,tc);
  }  
  
  img.updatePixels();
  return img;

}
PImage fsck(PImage img, int ix, int iy) { 
    if ( img == null ) return null;
    //img.loadPixels();
    int  iw = img1.width, ih = img1.height; 
//    float varx = map(ix,0,iw,0,100);
//    float vary = map(iy,0,ih,0,100);
    int ip = ix+iy*iw;
    recurseLameInsert(img,ix,iy,iw,ih,ip,color(0),0);
    
    return img;
}

void imgRect(int x, int y, int w, int h, color c) {
  int dx = leftMargin + x;
  int dy = menuHeight + y;
  noStroke();
  fill(color(255,0,0));
  rect(dx,dy,w,h); 
}
void recurseLameInsert(PImage img, int x, int y, int w, int h, int p, color pc, int lvl) {
  Log("recurse lvl "+lvl+" x:"+x+", y:"+y);
  if ( lvl>maxRecurse) return;
  int[][] neighbors = {
     { p-w-1, x-1, y-1 }, //tl
     { p-w, x, y-1 },//t
     { p-w+1,x+1, y-1 }, //tr
     { p-1, x-1, y }, //l
     { p+1, x+1, y }, //r
     { p+w-1, x-1, y+1 }, //bl
     { p+w, x, y+1}, //b
     { p+w+1, x+1, y+1} //br
  }; 
  int l = img.pixels.length;
  color c = img.pixels[p];

  for (int nl = 0; nl < 8; nl++) 
  {
     int tp = neighbors[nl][0];
     int tx = neighbors[nl][1];
     int ty = neighbors[nl][2];
     Log("testing pos"+tp+"("+tx+"x"+ty+")");
     if ( tp >= 0 && tp < l-1 ) { 
       color tc = img.pixels[tp];
       Log("brightness1: "+brightness(pc)+", brightness2: "+brightness(tc)+" diff: "+abs(brightness(tc)-brightness(pc))+", "+sensitivity);
       if ( abs(brightness(tc)-brightness(pc))<sensitivity) {
          recurseLameInsert(img,tx,ty,w,h,tp,pc,lvl+1);
       }
      
     }
  }
  img.loadPixels();
  img.pixels[p]=pc;
  img.updatePixels();
}

PImage lameSort(PImage img) {
  img.loadPixels();
  Boolean reverseit = random(1)>0.46;
  PImage r = createImage(img.width,img.height,RGB);
  for (int y = 0, h=img.height;y<h-1;y++) { 
    for (int x = 0,w=img.width;x<w;x+=w) {
      int pos = x+y*w; 
      int[] row = subset(img.pixels, pos, w);
      row = sort(row);
      if ( reverseit ) 
        row = reverse(row);
      arrayCopy(row,0,r.pixels, pos, w);
    }
  }  
  //img.updatePixels();
  return r;
}


PImage lameLace(PImage img) { 
  img.loadPixels();
  int w = img.width;
  for (int i = 0, l = img.pixels.length; i<l; i+=w) { 
    if ( i % (w*4) != 0 ) { 
      for (int u = i+w; u > i; u--) {
        if ( u < img.pixels.length ) 
            img.pixels[u]=color(0); 
      }
    }
  } 
  img.updatePixels();
  return img;
}

PImage vert(PImage img) {
   img.loadPixels();
   int diff = sensitivity;//xPercent;
  color last = color(0);
  color dc = img.pixels[0];
  int xw = (int)map(xPercent,0,100,pxWidth,maxPxWidth);// pxWidth;//2;//(int)map(mouseX,0,width,0,10);
  int yw = (int)map(yPercent,0,100,pxHeight,maxPxHeight);//pxHeight;//(int)map(mouseY,0,height,1,10);
  for ( int x = 0, w = img.width; x<w; x+= xw ) { 
    for (int y = 0, h = img.height; y<h; y+= yw ) { 
      int pos = x + y * w;
      color c = img.pixels[pos];
      if ( abs(brightness(c) - brightness(last)) > diff ) { 
        dc = c;
      }
      last = c;
      img.pixels[pos] = dc;
      //noStroke();
      //fill(dc);
      //rect(x,y,xw,yw);
    }
  }
  img.updatePixels();
  return img;
  
}
PImage edge002(PImage img) { 
    img.loadPixels();
    float[][] matrix = { 
  { -1, -1, -1},
  { -1,  9, -1},
  { -1, -1, -1} };
    PImage img3 = createImage(img1.width, img1.height, RGB);
    //skipping 1 pix so that there always is a pixel to compare with
    int xs = 1, ys = 1;
    for (int y = 1, h = img.height; y<h-1;) {
      for (int x = 1, w = img.width; x<w-1; x+=xs) { 
        float rsum = 0, gsum=0, bsum=0;
        for (int ky = -1; ky <=1; ky++) {
          for (int kx = -1; kx <= 1; kx++ ) { 
            int pos = (y+ky)*w+(x+kx);
            float r = red(img.pixels[pos]);
            float g = green(img.pixels[pos]);
            float b = blue(img.pixels[pos]);
            rsum += matrix[ky+1][kx+1] * r;
            gsum += matrix[ky+1][kx+1] * g;
            bsum += matrix[ky+1][kx+1] * b;
          }
        }
        if ( random(1)<0.6 ) {
          if ( random(1)>0.3 ) { 
             ys = (int)(1+(random(1)*10)); 
          }
          for (int yy = 0; yy < ys; yy++) { 
            for (int gl = 0, glm = (int)(1 + random(1)*300); gl < glm; gl++ ) {
              int ppos = (y+yy)*w+x+gl;
              if ( ppos < img3.pixels.length ) { 
                img3.pixels[ppos] = color(rsum,gsum,bsum);
              }
            }
          }
        }
      }
      y+=ys;
    }
    img3.updatePixels();
    return img3;
}
PImage edge001(PImage img) { 
  float[][] matrix = { 
  { -1, -1, -1},
  { -1,  9, -1},
  { -1, -1, -1} };
    img.loadPixels();
    //img2.loadPixels();//fixme: are these necessary?
    PImage img3 = createImage(img.width, img.height, RGB);
    //skipping 1 pix so that there always is a pixel to compare with
    int xs = 1, ys = 1;
    for (int y = 1, h = img.height-1; y<h-1;) {
      for (int x = 1, w = img.width-1; x<w-1; x+=xs) { 
        float rsum = 0, gsum=0, bsum=0;
        for (int ky = -1; ky <=1; ky++) {
          for (int kx = -1; kx <= 1; kx++ ) { 
            int pos = (y+ky)*w+(x+kx);
            float r = red(img.pixels[pos]);
            float g = green(img.pixels[pos]);
            float b = blue(img.pixels[pos]);
            rsum += matrix[ky+1][kx+1] * r;
            gsum += matrix[ky+1][kx+1] * g;
            bsum += matrix[ky+1][kx+1] * b;
          }
        }
        if ( random(1)<0.6 ) {
          if ( random(1)>0.3 ) { 
             ys = (int)(1+(random(1)*10)); 
          }
          for (int yy = 0; yy < ys; yy++) { 
            for (int gl = 0, glm = (int)(1 + random(1)*300); gl < glm; gl++ ) {
              img3.pixels[(y+yy)*w+x+gl] = color(rsum,gsum,bsum);
            }
          }
        }
      }
      y+=ys;
    }
    img3.updatePixels();
    return img3;
}


PImage edge000(PImage img) { 
  float[][] matrix = { 
  { -1, -1, -1},
  { -1,  9, -1},
  { -1, -1, -1} };
    img.loadPixels();
    //img2.loadPixels();//fixme: are these necessary?
    PImage img3 = createImage(img.width, img.height, RGB);
    //skipping 1 pix so that there always is a pixel to compare with
    for (int y = 1, h = img.height; y<h-1; y++) {
      for (int x = 1, w = img.width; x<w-1; x++) { 
        float rsum = 0, gsum=0, bsum=0;
        for (int ky = -1; ky <=1; ky++) {
          for (int kx = -1; kx <= 1; kx++ ) { 
            int pos = (y+ky)*w+(x+kx);
            float r = red(img.pixels[pos]);
            float g = green(img.pixels[pos]);
            float b = blue(img.pixels[pos]);
            rsum += matrix[ky+1][kx+1] * r;
            gsum += matrix[ky+1][kx+1] * g;
            bsum += matrix[ky+1][kx+1] * b;
          }
        }
        if ( random(1)<0.5 ) {
          for (int gl = 0, glm = (int)(1 + random(1)*100); gl < glm; gl++ ) {
            img3.pixels[y*w+x+gl] = color(rsum,gsum,bsum);
          }
        }
      }
    }
    img3.updatePixels();
    return(img3);
}          



PImage echoImage(PImage img, int amount, int mode) { 
  img.loadPixels();
  int[] bms =   { BLEND, ADD, SUBTRACT, DARKEST, LIGHTEST, DIFFERENCE, EXCLUSION, MULTIPLY, SCREEN, OVERLAY, HARD_LIGHT, SOFT_LIGHT, DODGE, BURN };
  String[] bmt = {  "BLEND", "ADD", "SUBTRACT", "DARKEST", "LIGHTEST", "DIFFERENCE", "EXCLUSION", "MULTIPLY", "SCREEN", "OVERLAY", "HARD_LIGHT", "SOFT_LIGHT", "DODGE", "BURN" };
  
  PImage result = new PImage(img.width, img.height, RGB);
  for ( int y = img.height - 1; y>=0; y-- ) { 
    for ( int x = img.width -1; x>=0; x-- ) { 
      int pos = x+y*img.width;
      int epos = x-amount+y*img.width;
      if ( epos < 0 ) 
       epos = img.width + epos;
      color c = img.pixels[pos];
      color ec = img.pixels[epos];
      color dc;
       switch(mode) { 
         case 0:
         dc = c+ec;
         break;
         default:
         if ( mode < bms.length ) { 
           dc = blendColor(c,ec,bms[mode-1]);
           Log("echo, blending with "+bmt[mode-1]+" from position "+amount);
         } else { 
          dc = c-ec; 
         }
       }
       result.pixels[pos] = dc;   
    }
  }
  result.updatePixels();
  return result;  
    
  
  
  
}










PImage vertBlurImage(PImage img, int iterations) {
  float[][] blur = { { 0, 0, 0 }, { 1.0/3.0, 1.0/3.0, 1.0/3.0 }, { 0, 0, 0 } };
  float [][] gFilter = blur;
  int matrixsize = gFilter.length;
  img.loadPixels(); // loadPixels();
  while ( iterations-->0  ) {
    for(int x = 0; x < img.width; x++) {
      for(int y = 0; y < img.height; y++) { // for(int y = 0; y < img.width; y++) {
        color c = blurImageConvolution(x, y, blur, matrixsize, img);
        int loc = x + y * img.width;
        img.pixels[loc] = c;
      }
    }
  }
  img.updatePixels(); // updatePixels();
  return img;
}
color blurImageConvolution(int x, int y, float[][] matrix, int matrixsize, PImage img) {
  float rtotal = 0.0;
  float gtotal = 0.0;
  float btotal = 0.0;
  int offset = matrixsize / 2;
  for(int i = 0; i < matrixsize; i++)
    for(int j = 0; j < matrixsize; j++) {
      int xloc = x + i - offset;
      int yloc = y + j - offset;
      int loc = xloc + yloc * img.width;
      loc = constrain(loc, 0, img.pixels.length - 1);
      rtotal +=   red(img.pixels[loc]) * matrix[i][j];
      gtotal += green(img.pixels[loc]) * matrix[i][j];
      btotal +=  blue(img.pixels[loc]) * matrix[i][j];
    }
  rtotal = constrain(rtotal, 0, 255);
  gtotal = constrain(gtotal, 0, 255);
  btotal = constrain(btotal, 0, 255);
  return color(rtotal, gtotal, btotal);
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


PImage hordu(PImage img, int xp, int yp) { 
  img.loadPixels();
  color last = color(0);
  color dc = img.pixels[0];
  int xw = (int)map(xp,0,100,1,10);
  int yw = 2;//(int)map(yp,0,100,1,10);
  int diff = (int)map(yp,0,100,80,10);
  int[] taken = new int[img.pixels.length];
  for (int y = 0, h = img.height; y<h; y+= yw ) { 
    for ( int x = 0, w = img.width; x<w; x+= xw ) { 
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


PImage mirror(PImage img, int xp, int yp) { 
  Log("mirrored");
  img.loadPixels();
  int pivotx = (int)((img.width-1)/2);
  int centery = (int)((img.height-1)*xp/100);
  
  for ( int x = 0, w = img.width; x<w; x++ ) { 
    for (int y = 0,h = img.height; y<h; y++ ) { 
        int pos = 0;
        int sx = x;
      if ( x > pivotx ) { 
        int tpos = x + y * w;
        sx = pivotx - abs(pivotx-x);
        int spos = sx + y*w;
        if ( spos >= 0 && spos < img.pixels.length ) 
          img.pixels[tpos] = img.pixels[spos];
      } 
    }
  }
  img.updatePixels();  
  return img; 
  
}

PImage ymirror(PImage img, int xp, int yp) { 
  Log("mirrored");
  img.loadPixels();
  int pivoty = (int)((img.height-1)/2);
  int centery = (int)((img.height-1)*xp/100);
  
  for (int y = 0,h = img.height; y<h; y++ ) { 
    for ( int x = 0, w = img.width; x<w; x++ ) { 
        int pos = 0;
        int sy = y;
      if ( y > pivoty ) { 
        int tpos = x + y * w;
        sy = pivoty - abs(pivoty-y);
        int spos = x + sy*w;
        if ( spos >= 0 && spos < img.pixels.length ) 
          img.pixels[tpos] = img.pixels[spos];
      } 
    }
  }
  img.updatePixels();  
  return img; 
  
}



PImage bw(PImage img) { 
   img.loadPixels();
   for (int i = 0, l = img.pixels.length; i<l; i++ ) { 
     img.pixels[i] = color(brightness(img.pixels[i]));
   } 
   img.updatePixels();
   return img;
  
}

PImage offsetImage(PImage img, int xp, int yp) { 
  Log("offsetting at "+xp+"%");
  int xoffset = (int)map(img.width*xp/100,0,img.width,img.width,0);
  img.loadPixels();
  PImage result = createImage(img.width,img.height,RGB);
  for (int y=0, h=img.height; y<h; y++) { 
   for ( int x =0, w=img.width; x<w; x++) { 
     int tpos = x+y*w;
     int sx = x + xoffset;
     if ( sx >= w ) { 
       sx = sx - w; 
     }
     int spos = sx+y*w;
     if ( spos < img.pixels.length && spos > 0 )
       result.pixels[tpos] = img.pixels[spos];     
   }
  } 
  result.updatePixels();
  return result; 
}

PImage rgbOffsetImage(PImage img, int xp, int yp, int mod) { 
  Log("rgb offsetting at "+xp+"%");
  int xoffset = (int)(img.width*xp/100);
  img.loadPixels();
  PImage result = createImage(img.width,img.height,RGB);
  for (int y=0, h=img.height; y<h; y++) { 
   for ( int x =0, w=img.width; x<w; x++) { 
     int tpos = x+y*w;
     int sx = x + xoffset;
     if ( sx >= w ) { 
       sx = sx - w; 
     }
     int spos = sx+y*w;
     if ( spos < img.pixels.length && spos > 0 ) {
//             result.pixels[tpos] = img.pixels[spos];     
        color oc = img.pixels[tpos];
        color sc = img.pixels[spos];
        int r = (int) ( mod == 0 ? ( red(oc)<180 ? (int)constrain(red(oc)+red(sc),0,255) : (int)red(oc) ) : red(oc));
        int g = (int) ( mod == 1 ? ( green(oc)<180 ? (int)constrain(green(oc)+green(sc),0,255) : (int)green(oc) ) : green(oc));
        int b = (int) ( mod == 2 ? ( blue(oc)<180 ? (int)constrain(blue(oc)+blue(sc),0,255) : (int)blue(oc) ) : blue(oc));
        result.pixels[tpos] = color(r,g,b);
     }
   }
  } 
  result.updatePixels();
  return result; 
}


PImage warpImage(PImage img, int xp, int yp) {
  int direction = (int)map(xp,0,100,-5,5); 
  if (direction == 0) direction = 1;
  PImage result = new PImage(img.width,img.height);
  img.loadPixels();
  for (int y=0, h=img.height; y<h; y++) { 
   for ( int x =0, w=img.width; x<w; x++) { 
     int pos = (int)(x + y*(w)); 
     int gpos = (int)(x + y*(w+direction));
     if ( gpos < img.pixels.length ) { 
       result.pixels[pos] = img.pixels[gpos];
     } else { 
       result.pixels[pos] = img.pixels[pos];       
     }
   }
  }
  result.updatePixels(); 
  return result;
}



PImage brighter(PImage img, int xp, int yp) {
  float enhance = map(xp,0,100,0,2);
  img.loadPixels();
  for ( int i = 0; i < img.width; i++) {
    // Begin loop for rows
    for ( int j = 0; j < img.height; j++) {
      int loc = i + j*img.width;
      color c = img.pixels[loc];
      int r = (int)constrain(red(c)*enhance,0,255);
      int g = (int)constrain(green(c)*enhance,0,255);
      int b = (int)constrain(blue(c)*enhance,0,255);
      c = color(r,g,b);
      img.pixels[loc]=c;
    }
  }
  img.updatePixels();
  return img;
}



PImage faces(PImage img, PImage mergeImage, int xPerc) { 
  return img;/*
  OpenCV opencv = new OpenCV(this, img);
  opencv.loadCascade(OpenCV.CASCADE_FRONTALFACE);
  Rectangle[] faces = opencv.detect();
  Log("Faces detected "+faces.length+" faces");
  PImage result = createImage(img.width,img.height,RGB);
  img.loadPixels();
  if ( mergeImage != null ) {
    mergeImage.loadPixels();
  } else {
    mergeImage = createImage(img.width, img.height, RGB);
  }
  result.copy(mergeImage,0,0,img.width,img.height,0,0,img.width,img.height);
  for ( int i = 0; i < faces.length; i++) {
    for (int x = faces[i].x + (int)(faces[i].width/5), w = x+faces[i].width; x<w; x++) { 
      for (int y = faces[i].y, h = y+faces[i].height; y<h; y++) { 
        int ctrx = faces[i].width/2 + faces[i].x;
        int ctry = faces[i].width/2 + faces[i].y;
        int pos = x + y*result.width;
        print("distance: "+dist(x,y,ctrx,ctry)+", centery:"+ctry+", ");
        color c = img.pixels[pos];
        color bc = result.pixels[pos];
        color dc = blendColor(c,bc,blendMode);
        result.pixels[pos] = dist(x,y,ctrx,ctry)>ctry/3 ? bc : dc;

      }
    }
  }
  result.updatePixels();
  return(result);*/  
}


PImage superLameGlitch(PImage img, int xp, int yp) { 
  int g = 10;
  int offset = 0;
  PImage result = createImage(img.width,img.height,RGB);
  for (int i = 0; i<result.pixels.length; i++) { 
     if ( i % ( map(xp,0,100,1,20) * img.width ) == 0 )  {
         g = (int)(random(1)*30);
         offset += g;
         i+=g;
     }
     if ( i + offset < result.pixels.length )
     result.pixels[i]=img.pixels[i+offset];
     
  }
  return result;
}

PImage stretch(PImage img, int xp, int yp) { 
   img.loadPixels();
   for ( int x = 0; x < img.width; x++ ) { 
     for ( int y = 0; y < img.height; y++ ) { 
       int pos = x + y * img.width;
       color c = img.pixels[pos];
       int sp = (x*x);
       int ep = sp+x;
       for ( int np = sp; np<ep; np++ ) { 
         np = np + y * img.width;
         if ( np < img.pixels.length -1 ) {
           img.pixels[np] = c;
         } 
       }
     }
   }
   img.updatePixels();
   return img;
}

PImage stretch3(PImage img, int xp, int yp) { 
   img.loadPixels();
   PImage result = createImage(img.width, img.height, RGB );
   result.loadPixels();
   for ( int y = 0; y < img.height; y++ ) { 
     for ( int x = 0; x < img.width; x++ ) { 
       int pos = x + y * img.width;
       color c = img.pixels[pos];
       int l = (2*x)+1;
       int sp = (int)((2*x)*((2*x)+1)/2);
       result.pixels[pos] = c;
       if ( sp >= img.width ) { 
         x = img.width;
       } else { 
         for ( int i = 0; i < l; i++ )  {
           if ( sp+i < img.width ) { 
             int pp = (int)( ( sp+i ) + y * img.width );
             result.pixels[pp] = c;
           }
         }  
       }
     }
   }
   result.updatePixels();
   return result;
}

PImage stretch2(PImage img, int xp, int yp) { 
   img.loadPixels();
   PImage result = createImage(img.width, img.height, RGB );
   result.loadPixels();
   for ( int y = 0; y < img.height; y++ ) { 
     for ( int x = 0; x < img.width; x++ ) { 
       int pos = x + y * img.width;
       color c = img.pixels[pos];
       int l = x+1;
       int sp = (int)(x*(x+1)/2);
       result.pixels[pos] = c;
       if ( sp >= img.width ) { 
         x = img.width;
       } else { 
         for ( int i = 0; i < l; i++ )  {
           if ( sp+i < img.width ) { 
             int pp = (int)( ( sp+i ) + y * img.width );
             result.pixels[pp] = c;
           }
         }  
       }
     }
   }
   result.updatePixels();
   return result;
}




PImage diag(PImage img, int xp, int yp) { 
 img.loadPixels();
 color pc = color(0);
 int mindiff = (int)map(xp,0,100,1,100);
 for (int i = 0; i < img.width; i++ ) { 
   int x = i;
   int y = 0;
   while ( x >= 0 && y >= 0) { 
     //println("x: "+x+", y: "+y);
     int pos = x + y * img.width;
     if ( pos < img.pixels.length ) { 
       color c = img.pixels[pos];
       if ( abs(brightness(pc)-brightness(c)) > mindiff ) {
         pc = c;
       }
       img.pixels[pos] = pc;
     }
     y++;
     x--;
   } 
 }
 img.updatePixels();
 return img;
} 
  
PImage posterize(PImage img, int xp, int yp) { 
  int level = (int) map(xp,0,100,2,32);
  Log("Posterize reducing colors to "+level);
  img.loadPixels();
  img.filter(POSTERIZE,level);
  img.updatePixels();
  return img; 
}
  
PImage filtr(PImage img, int xp, int yp) {
  filidx = (int) map(xp,0,100,0,fils.length);
  Log("Filter set to "+filTitles[filidx]);
  return flt(img, fils[filidx], yp, 0);
}
PImage flt(PImage img, int fil, int xp, int yp) { 
  img.loadPixels();
  switch ( fil ) {
     case THRESHOLD:
     img.filter(THRESHOLD,map(xp,0,100,0,1));
     break;
     case BLUR:
       img.filter(BLUR,(int)map(xp,0,100,1,10));
     break; 
     case POSTERIZE:
       int pl = (int) map(xp,0,100,2,32);
       Log("Filter posterize at "+pl);
       img.filter(POSTERIZE,pl);
     break;
     default: //GRAY, INVERT, OPAQUE, ERODE, DILATE   
       img.filter(fil);
     break; 
  }
  //img.filter(POSTERIZE,level);
  img.updatePixels();
  return img; 
}
  
  
PImage sinsin(PImage img, int xp, int yp) { 
   img.loadPixels();
   float theta = 0;
   PImage result = createImage(img.width, img.height, RGB);
   result.loadPixels();
   for ( int y = 0; y < img.height; y++ ) { 
     for ( int x = 0; x < img.width; x++ ) { 
       float direction = sin(theta);//(int)map(sin(theta),-1,1,0,1);
       int posy = (int)(y*direction);
       int dy = y;
       if ( posy >= 0 && posy < img.height ) dy = posy;
       int pos = x + y * img.width;
       int sp = pos;
       int pospos = x + dy * img.width;
       if ( pospos >= 0 && pospos < img.pixels.length ) sp = pospos;
       result.pixels[sp] = img.pixels[pos];
       
       theta += TWO_PI/60;
     }
   }
   result.updatePixels();
   return result;
}

PImage sinsin2(PImage img, int xp, int yp) { 
   img.loadPixels();
   float theta = 0;
   color c = color(0), pc = color(0);
   int diff = 10;
   PImage result = createImage(img.width, img.height, RGB);
   result.loadPixels();
   for ( int y = 0; y < img.height; y++ ) { 
     for ( int x = 0; x < img.width; x++ ) { 
       int pos = x + y * img.width;
       if ( abs(brightness(img.pixels[pos])-brightness(pc)) > diff ) { 
         pc = c = img.pixels[pos];
       }  
       float direction = sin(theta);//(int)map(sin(theta),-1,1,0,1);
       int posy = (int)(y*direction);
       int dy = y;
       if ( posy >= 0 && posy < img.height ) dy = posy;
       int sp = pos;
       int pospos = x + dy * img.width;
       if ( pospos >= 0 && pospos < img.pixels.length ) sp = pospos;
       result.pixels[sp] = c;
       
       theta += TWO_PI/60;
     }
   }
   result.updatePixels();
   return result;
}

PImage sinsin3(PImage img, int xp, int yp) { 
   img.loadPixels();
   float theta = 0;
   color c = color(0), pc = color(0);
   int diff = 30;
   PImage result = createImage(img.width, img.height, RGB);
   result.loadPixels();
   for ( int y = 0; y < img.height; y++ ) { 
     for ( int x = 0; x < img.width; x++ ) { 
       int pos = x + y * img.width;
       c = img.pixels[pos];
       float direction = sin(theta);//(int)map(sin(theta),-1,1,0,1);
       int posy = (int)(10*y*direction);
       int dy = y;
       if ( posy >= 0 && posy < img.height ) dy = posy;
       int sp = pos;
       int pospos = x + dy * img.width;
       if ( pospos >= 0 && pospos < img.pixels.length ) sp = pospos;

       if ( abs(brightness(c)-brightness(pc)) > diff ) { 
         pc = c;
       }  
       result.pixels[sp] = c;
       
       theta += TWO_PI/60;
     }
   }
   result.updatePixels();
   return result;
}

void undo() { 
         if ( historyIdx > 0 ) {
           PImage pimg = history.get(historyIdx-=1);
           img1.copy(pimg,0,0,pimg.width,pimg.height,0,0,img1.width,img1.height);
         }
}


PImage resizeimg(PImage img, int w, int h) { 
  PImage result = createImage(w,h,RGB);
  result.copy(img,0,0,img.width,img.height,0,0,result.width,result.height);
 return result; 
}

PImage doubleimg(PImage img, int xp, int yp, int mod) { 
   img.loadPixels();
   PImage result;
   if ( mod == 0 ) { 
     result = createImage(img.width*2,img.height,RGB);
   result.copy(img,0,0,img.width,img.height,0,0,img.width,img.height);
   result.copy(img,0,0,img.width,img.height,img.width,0,img.width,img.height);
   } else { 
     result = createImage(img.width,img.height*2,RGB);
   result.copy(img,0,0,img.width,img.height,0,0,img.width,img.height);
   result.copy(img,0,0,img.width,img.height,0,img.height,img.width,img.height);     
   }
   result.updatePixels();
   return result;
}


PImage lameglitch2(PImage img, int xp, int yp) {
 int levels = (int)map(xp,0,100,1,1000);
 int maxdiv = (int)map(yp,0,100,10,1);
 PImage result = createImage(img.width, img.height, RGB);
 int i = 0, sx, sy, sw, sh, tx, ty, tw, th;
 if (maxdiv > 8 ) { 
   result.copy(img,0,0,img.width,img.height,0,0,img.width, img.height); 
 }
 //img.loadPixels();
 while ( ++i < levels ) { 
   sw = (int)(random(1)*img.width/maxdiv);
   sx = (int)((img.width-sw)*random(1));
   sh = (int)(random(1)*img.height/maxdiv);
   sy = (int)((img.height-sh)*random(1));
   tw = (int)(random(1)*img.width/maxdiv);
   tx = (int)((img.width-sw)*random(1));
   th = (int)(random(1)*img.height/maxdiv);
   ty = (int)((img.height-sh)*random(1));
   result.copy(img,sx,sy,sw,sh,tx,ty,tw,th);
 }
 return result;
}



PImage jarimg(PImage img, int xp, int yp) { 
  int glitchWidth = (int)map(xp,0,100,0,img.width);
  int glitchTop = (int)map(yp,0,100,0,img.height);
  PImage result = createImage(img.width, img.height, RGB);
  result.copy(img,0,0,img.width,img.height,0,0,result.width,result.height);
  result.copy(img,-img.width+glitchWidth,glitchTop,img.width,img.height-glitchTop,-img.width+glitchWidth,glitchTop,img.width,img.height-glitchTop);
  result.copy(img,glitchWidth,glitchTop,img.width,img.height-glitchTop,glitchWidth,glitchTop,img.width,img.height-glitchTop);
  return result; 
}



PImage brDrift(PImage img, int xp, int yp) { 
   img.loadPixels();
   PImage result = createImage(img.width,img.height,RGB);
   result.loadPixels();
   Boolean origfil = xp > 50;
   Boolean smear = xp < 25 || xp > 75;
   int div = (int)map(yp,0,100,10,1);
   for ( int x = 0; x < img.width; x++ ) { 
     for ( int y = 0; y < img.height; y++ ) { 
       int pos = x + y * img.width;
       color c = img.pixels[pos];
       int b = (int)(red(c)+green(c)+blue(c));//(int)brightness(c);
       int tx = (int)constrain(x+map(b,0,765,-img.width/div,img.width/div),0,img.width-1);
       int tpos = tx + y * img.width;
       if ( origfil && result.pixels[pos] == 0 ) result.pixels[pos]=c;
       result.pixels[tpos]=c;
       if ( smear ) { 
          if ( tpos < pos ) {
            for ( int i = tpos; i<pos; i++ ) { 
              if ( result.pixels[i] == 0 || result.pixels[i] == 255) 
                result.pixels[i] = c;
             // else 
             //   result.pixels[i] = ( c/2+result.pixels[i]/2);
            }
          } else { 
            for ( int i = pos; i<tpos; i++ ) { 
              if ( result.pixels[i] == 0  || result.pixels[i] == 255 ) 
                result.pixels[i] = c;
             // else 
             //   result.pixels[i] = ( c/2+result.pixels[i]/2);
            }
          }
       }
     }
   }
   result.updatePixels();
   return result;
}



PImage slicer(PImage img,int xp, int yp) 
{
  img.loadPixels();
  int y = (int)(img.height*yp/100);
  int x = (int)(img.width*xp/100);
  PImage result = createImage(img.width,img.height,RGB);
  //nonglitched part:
  result.copy(img,0,0,img.width,y,0,0,result.width,y);
  //glitched lefthalf
  result.copy(img,x,y,img.width-x,img.height-y,0,y,result.width-x,result.height-y);
  //glitched righthalf
  result.copy(img,0,y,x,img.height-y,img.width-x,y,x,result.height-y);
  return result;//
}



PImage trans(PImage img, int xp, int yp) { 
 img.loadPixels();
 int tranny = (int)map(xp,0,100,0,255);
 for (int i=0,l=img.pixels.length; i<l; i++) { 
   color c = img.pixels[i];
   img.pixels[i]=color(red(c),green(c),blue(c),tranny); 
 }
 img.updatePixels();
 return img;
}


PImage selfie(PImage img, int px, int py) { 
  PImage result = createImage(img.width,img.height,RGB);
  img.loadPixels();
  result.loadPixels();
  int gridh = (int)map(px,0,100,10,200);//(int)(img.height/5));
  int gridw = (int)map(py,0,100,10,200);//(int)(img.width/5));
  result = fractimg(result,img,gridw,gridh,1);
  result.updatePixels();
  return result;
}
PImage fractimg(PImage result, PImage img, int gridw, int gridh, int iter) {
  int minw = 10, minh = 10;
  for ( int y = 0, h = img.height; y<h; y+=gridh) { 
    for ( int x = 0, w = img.width; x<w; x+=gridw) { 
      int pos = x+y*w;
      color c = img.pixels[pos];
      int[] quadavg = avgbright(img,x,y,gridw,gridh);
      PImage quad = createImage(gridw,gridh,RGB);
      quad.copy(img,0,0,w,h,0,0,quad.width-1,quad.height-1);
      quad = colorize(quad,c);
      result.copy(quad,0,0,quad.width,quad.height,x,y,gridw,gridh);
    }
  }
  result.updatePixels();
  return result;
}


int[] avgbright(PImage img, int x, int y, int w, int h) {
  int totalr = 0, totalg = 0, totalb = 0, totalbr = 0, totalnum = 0;
  for ( int row = y; row < y+h; row++ ) { 
   for ( int col = x; col < x+w; col++ ) { 
       color c = img.pixels[col+row*w];
       totalbr += brightness(c);
       totalr += red(c);
       totalg += green(c);
       totalb += blue(c);
       totalnum += 1;
   }
  }
  //print((int)(total/totalnum)+", ");
  return new int[] { totalr, totalg, totalb, totalbr };// (int)(total/totalnum);
  
}


PImage colorize(PImage img, color c) { 
  img.loadPixels();
 for ( int i = 0, l = img.pixels.length; i<l; i++ ) {
   color oc = img.pixels[i];
   int r = (int)map(brightness(oc),0,255,0,red(c));
   int g = (int)map(brightness(oc),0,255,0,green(c));
   int b = (int)map(brightness(oc),0,255,0,blue(c));
   img.pixels[i] = color(r,g,b);
 } 
 img.updatePixels();
 return img;
}


PImage extractLayer(PImage img, int mod) { 
  img.loadPixels();
  if ( mod == 0 ) { 
    for ( int i = 0; i<img.pixels.length; i++) { 
      img.pixels[i] = color(red(img.pixels[i]),0,0);
    }
  } else if ( mod == 1 ) { 
    for ( int i = 0; i<img.pixels.length; i++) { 
      img.pixels[i] = color(0,green(img.pixels[i]),0);
    }
  } else { 
    for ( int i = 0; i<img.pixels.length; i++) { 
      img.pixels[i] = color(0,0,blue(img.pixels[i]));
    }
  }
  img.updatePixels();
  return img;
}





PImage drawPoints(PImage img, ArrayList<PVector> pts, color c) { 
  img.loadPixels();
  for ( int i = 0; i<pts.size(); i++) { 
    int pos = (int)(pts.get(i).x+pts.get(i).y*(img.width)); 
    if ( pos < img.pixels.length ) {
     img.pixels[pos] = c;
    }
  } 
  img.updatePixels();
  return img;
  
}


/* Use PVector instead 
class P { 
  P(int _x, int _y) { 
    this.x = _x;
    this.y = _y;
  }
  P(int _x, int _y, int _z) { 
    this.x = _x;
    this.y = _y;
    this.z = _z;
  }
  int x;
  int y;
  int z;
}
*/
ArrayList<PVector> edgeDetect(PImage img, int sensitivity) { 
  img.loadPixels();
  ArrayList<PVector> result = new ArrayList<PVector>();
  for ( int y = 1, h=img.height; y<h-1; y++) { //skip 1 for the edges (can't compare to values beyond the edge ;))
   for ( int x = 1, w=img.width; x<w-1; x++) { 
     color p = img.pixels[ x + y*w ];
     color tl = img.pixels[ (x-1) + (y-1)*w ];
     color t = img.pixels[ x + (y-1)*w ];
     color tr = img.pixels[ (x+1) + (y-1)*w ];
     color l = img.pixels[ (x-1) + y*w ];
     color r = img.pixels[ (x+1) + y*w ];
     color bl = img.pixels[ (x-1) + (y+1)*w ];
     color b = img.pixels[ x + (y+1)*w ];
     color br = img.pixels[ (x+1) + (y+1)*w ];
     color[] tests = { tl, t, tr, l, r, bl, b, br };
     for (int i = 0; i<tests.length; i++) { 
       if ( abs(brightness(p)-brightness(tests[i])) > sensitivity ) { 
         //print("F: "+i+"!! x:"+x+", y:"+y+", diff:"+abs(brightness(p)-brightness(tests[i]))+", ");
        result.add(new PVector(x,y,0));
        i=tests.length; // already a point, skipping to next
       }      
       //println( ""+i+": "+x+"x"+y+": "+abs( p - tests[i] ) );
     }

   }
  } 
     println("got "+result.size()+" points");
     return result; 
 
}




ArrayList<PVector> interestingPattern(PImage img, int sensitivity) { 
  img.loadPixels();
  ArrayList<PVector> result = new ArrayList<PVector>();
  for ( int y = 1, h=img.height-1; y<h; y++) { 
   for ( int x = 1, w=img.width-1; x<w; x++) { 
     color p = img.pixels[ x + x*2 ];
     color tl = img.pixels[ (x-1) + x*(y-1) ];
     color t = img.pixels[ x + x*(y-1) ];
     color tr = img.pixels[ (x+1) + x*(y-1) ];
     color l = img.pixels[ (x-1) + x*y ];
     color r = img.pixels[ (x+1) + x*y ];
     color bl = img.pixels[ (x-1) + x*(y+1) ];
     color b = img.pixels[ x + x*(y+1) ];
     color br = img.pixels[ (x+1) + x*(y+1) ];
     color[] tests = {  t, tr, l, r, bl, b, br };
     for (int i = 0; i<tests.length; i++) { 
       if ( abs(brightness(p)-brightness(tests[i])) > sensitivity ) { 
        // print("F: "+i+"!! x:"+x+", y:"+y+", diff:"+abs(brightness(p)-brightness(tests[i]))+", ");
        result.add(new PVector(x,y,0));
        i=tests.length; // already a point, skipping to next
       }      
       //println( ""+i+": "+x+"x"+y+": "+abs( p - tests[i] ) );
     }

   }
  } 
     println("got "+result.size()+" points");
     return result; 
 
}

ArrayList<PVector> bresenTriangle(PVector p1, PVector p2, PVector p3) { 
  ArrayList<PVector> result = new ArrayList<PVector>();
  result.addAll(bresenham(p1,p2));
  result.addAll(bresenham(p2,p3));
  result.addAll(bresenham(p3,p1));
  //result.addAll(bresenham(p1,p2));
  return result; 
  
}


ArrayList<PVector> bresenham(PVector p1, PVector p2) {
   ArrayList<PVector> result = new ArrayList<PVector>();
   int x1 = (int)p1.x;
   int x2 = (int)p2.x;
   int y1 = (int)p1.y;
   int y2 = (int)p2.y;
   int dx = abs(x2-x1);
   int dy = abs(y2-y1);
   int sx = (x1 < x2 ) ? 1 : -1;
   int sy = (y1 < y2 ) ? 1 : -1;
   int err = dx-dy;
   while (!((x1==x2) && (y1==y2))) {
     result.add(new PVector(x1,y1)); 
     int err2 = 2*err;
     if (err2 >-dy) { 
        err -= dy;
        x1 += sx;
     }
     if (err2 < dx) { 
        err += dx;
        y1 += sy;
     }   
   }
   return result;
  
}

PImage spielerei(PImage img, int xp, int yp) { 
   img.loadPixels();
   int cx = img.width/2;
   int cy = img.width/2;
   int maxdist = (int)dist(0,0,cx,cy);
   for ( int x = 0; x < img.width; x++ ) { 
     for ( int y = 0; y < img.height; y++ ) { 
       int pos = x + y * img.width;
       //println("WTF"+dist(x,y,cx,cy));
     }
   }
   img.updatePixels();
   return img;
}


/*
void drawTriangle(PVector p1, PVector p2, PVector p3, color c)  {
  stroke(c);
  strokeWeight(1); 
  beginShape();
  vertex(p1.x,p1.y);
  vertex(p2.x,p2.y);
  vertex(p3.x,p3.y);
  vertex(p1.x,p1.y);
  endShape();
  
}*/

Boolean isInTriangle(PVector pt, PVector tp1, PVector tp2, PVector tp3) {
  // from stack: http://stackoverflow.com/questions/14669614/finding-whether-a-point-is-within-a-triangle
  float x = pt.x, y = pt.y;
  float x1 = tp1.x, y1 = tp1.y;
  float x2 = tp2.x, y2 = tp2.y;
  float x3 = tp3.x, y3 = tp3.y;

  // no need to divide by 2.0 here, since it is not necessary in the equation
  double ABC = Math.abs (x1 * (y2 - y3) + x2 * (y3 - y1) + x3 * (y1 - y2));
  double ABP = Math.abs (x1 * (y2 - y) + x2 * (y - y1) + x * (y1 - y2));
  double APC = Math.abs (x1 * (y - y3) + x * (y3 - y1) + x3 * (y1 - y));
  double PBC = Math.abs (x * (y2 - y3) + x2 * (y3 - y) + x3 * (y - y2));

  boolean isInTriangle = ABP + APC + PBC == ABC;

  return isInTriangle;
}

PImage switchcolor(PImage img, int xp, int yp) { 
  PImage result = createImage(img.width,img.height,RGB);
  img.loadPixels();
  result.loadPixels();

  for ( int i = 0; i< img.pixels.length; i++ ) { 
    color c = img.pixels[i];
    if ( xp <= 20 ) 
      result.pixels[i] = color(green(c),blue(c),red(c),alpha(c));
    else if ( xp <= 40 )
      result.pixels[i] = color(blue(c),red(c),green(c),alpha(c));
    else if ( xp <= 60 )
      result.pixels[i] = color(red(c),blue(c),green(c),alpha(c)); 
    else if ( xp <= 80 )
      result.pixels[i] = color(alpha(c),blue(c),green(c),red(c)); 
    else   
      result.pixels[i] = color(blue(c),alpha(c),green(c),red(c)); 
  }
  result.updatePixels();
  return result;  
}


////////////////////////////////////////////

PImage terminator(PImage img, int xp, int yp) {
  String[] terminators = { 
    "../../../clipart/terminator.png",
    "../../../clipart/vcr.png",
    "../../../clipart/vcr2.png",
    "../../../clipart/vcr3.png",
    "../../../clipart/vcr4.png",
    "../../../clipart/vcr5.png",
    "../../../clipart/hud1.png",
    "../../../clipart/hud2.png",
    "../../../clipart/hud2.jpg",
    "../../../clipart/hud3.jpg",
    "../../../clipart/hud4.png",
    "../../../clipart/game1.png",
    "../../../clipart/game2.jpg",
   "../../../clipart/game3.jpg",
   "../../../clipart/game4.png",
   "../../../clipart/game5.jpg",
   "../../../clipart/test1.jpg",
   "../../../clipart/robo1.png",
   "../../../clipart/robo2.png",
   "../../../clipart/terminator2.png" };
  //String arnold ="../../../clipart/vcr2.png";// terminators[(int)(random(1)*terminators.length)];
  String arnold = terminators[(int)map(xp,0,101,0,terminators.length)];
  PImage oImg = loadImage(arnold);
  PImage result = createImage(img.width,img.height,RGB);
  int minbr = 10;
  int ow = oImg.width;
  int oh = oImg.height;
  for ( int x = 0, w = img.width; x<w; x++ ) {
    for ( int y = 0, h = img.height; y<h; y++ ) { 
      int ox = (int)map(x,0,w,0,ow);
      int oy = (int)map(y,0,h,0,oh);
      color oc = oImg.pixels[ox+oy*ow];
      int pos = x+y*w;
      color c = img.pixels[pos];
      int xxx = (int)map(yp,0,100,1,254);
      oc = color(red(oc),green(oc),blue(oc),xxx);
      //c = color(red(c),green(c),blue(c),255-xxx);
      if ( brightness(oc) > minbr ) {
        result.pixels[pos] = blendColor(c,oc,BLEND); 
      } else { 
        result.pixels[pos] = c; 
      }
    }
  }
  result.updatePixels();
  return result;
}

PImage template(PImage img, int xp, int yp) { 
   img.loadPixels();
   for ( int x = 0; x < img.width; x++ ) { 
     for ( int y = 0; y < img.height; y++ ) { 
       int pos = x + y * img.width;
     }
   }
   img.updatePixels();
   return img;
}
