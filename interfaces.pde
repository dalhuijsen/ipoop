interface IRunnableEffect { 
  PImage run(PImage img, float xPercent, float yPercent); 
}


interface IRunnableMergeEffect { 
  PImage run(PImage img1, PImage img2, float xPercent, float yPercent);
}



