class Slicer implements IRunnableEffect { 
  PImage run(PImage img,float xp, float yp) 
  {
    img.loadPixels();
    int y = (int)map(yp,0,100,0,img.height);//(img.height*yp/100);
    int x = (int)map(xp,100,0,0,img.width);//(img.width*xp/100);
    PImage result = createImage(img.width,img.height,RGB);
    result.copy(img,0,0,img.width,y,0,0,result.width,y);
    result.copy(img,x,y,img.width-x,img.height-y,0,y,result.width-x,result.height-y);
    result.copy(img,0,y,x,img.height-y,img.width-x,y,x,result.height-y);
    return result;//
  }
}

