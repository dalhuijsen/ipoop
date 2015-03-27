ArrayList<Recipe> _recipes = new ArrayList<Recipe>();
PImage img1,img2;
int drawCounter = 0;
void setup() { 
	img1 = loadImage("img.jpg");
        size(800,600);//size(img1.width,img1.height);
	background(black);
	frameRate(1);
	
}
void draw() { 
  // in it's most simple form:
  Recipe test = new Recipe();
  test.RunnableEffect = new SinSin();
  test.XPercent = 33.3;
  test.YPercent = 1.618;
  img1 = test.run(img1);
  image(img1,0,0);

  // or, for some automation: (uncomment the bit below about "now for some fun" to enable :) 
	if (img1 != null ) { 
                for (int i = 0, l = _recipes.size(); i<l; i++) { 
                    Recipe recipe = _recipes.get(i);
  			if ( recipe.RunnableMergeEffect != null ) { 
				//kewl
			} else if ( recipe.RunnableEffect != null ) { 

			}

		}
	}	

	drawCounter++;
	if ( drawCounter >= _recipes.size() ) { 
		noLoop();
	}
}
