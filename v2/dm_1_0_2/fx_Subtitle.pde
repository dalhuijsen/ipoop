class Subtitle implements IConfigurableEffect { 
  color _fgColor, _bgColor;
  String _txt;
  HashMap<String, Object> _conf;// = new HashMap<String, Object>();
  HashMap<String,ConfigDefinition> _defaults;// = new HashMap<String, Object>();

  Subtitle() { 
    _conf = new HashMap<String, Object>();
    _defaults = new HashMap<String,ConfigDefinition>();
    _conf.put( "fgColor", color(0xff, 0xff, 0x00) );
//    _defaults.put( "fgColor", new Integer[] { color(0x00,0x00,0x00), color(0xff,0xff,0xff) });
    _defaults.put( "fgColor", new ConfigDefinition("color","fgColor",null,null,color.class) );
    _conf.put( "bgColor", color(0x00, 0x00, 0x00) );
    _conf.put( "txt", "we're no longer in kansas Dorothy" );
    loadConf();
  }  
  void config(HashMap<String, Object> conf) { 
    this._conf = conf;
    loadConf();
  }
  HashMap<String, Object> config() { 
    return this._conf;
  }
  
  HashMap<String,ConfigDefinition> defaults() { 
    return _defaults;
 
  }

  void loadConf() { 
    _fgColor = (Integer)_conf.get("fgColor");
    _bgColor = (Integer)_conf.get("bgColor");
    _txt = (String)_conf.get("txt");
  }
  PImage run(PImage img, float xp, float yp) {

    PGraphics buffer = createGraphics(img.width, img.height);
    buffer.beginDraw();
    PFont subFont = loadFont("mono.vlw");
    buffer.textFont(subFont); 
    buffer.textAlign(LEFT, TOP);

    buffer.image(img, 0, 0);
    float phi = 1.618;
    int ts = buffer.height/22;
    int tl = _txt.length();
    int tw = tl * (int)(ts/phi);
    int tx = (buffer.width-tw)/2;
    int ty = buffer.height-(int)(2*ts); 

    buffer.textSize(ts);
    buffer.fill(_bgColor);

    buffer.text(_txt, tx-1, ty-1);
    buffer.text(_txt, tx-1, ty);
    buffer.text(_txt, tx+1, ty);

    buffer.text(_txt, tx+1, ty+1);
    buffer.fill(_fgColor);
    //ts = ts -2;
    tx = tx - 2;
    if ( tx < 0 ) tx = 1;
    buffer.textSize(ts);
    buffer.text(_txt, tx, ty);
    buffer.endDraw();
    PImage result = (PImage)buffer;
    return result;
  }
}


