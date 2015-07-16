class AuPhaser implements IEffect { 

  PImage run(PImage img, float xp, float yp) { 

    /* EffectPhaser::ProcessInitialize */
    //FIXME: no idea what to set these at:
    float mSampleRate = 128;
    //constants
    float phaserlfoshape = 4.0;
    int lfoskipsamples = 20; //how many samples are processed before recomputing lfo
    int numStages = 24;
    //defaults
    float mFreq = 0.4, mPhase = 0;
    int mStages = 2, mDryWet = 128, mDepth = 100, mFeedback = 0;


    //getParams
    /*
    Phaser Parameters
     
     mFreq       - Phaser's LFO frequency
     mPhase      - Phaser's LFO startphase (radians), needed for stereo Phasers
     mDepth      - Phaser depth (0 - no depth, 255 - max depth)
     mStages     - Phaser stages (recomanded from 2 to 16-24, and EVEN NUMBER)
     mDryWet     - Dry/wet mix, (0 - dry, 128 - dry=wet, 255 - wet)
     mFeedback   - Phaser FeedBack (0 - no feedback, 100 = 100% Feedback,
     -100 = -100% FeedBack)
     */
    mFreq = map(xp, 0, 100, 0.4, 4.0);
    mDryWet = (int) map(xp, 0, 100, 0, 255);
    mDepth = (int) map(xp, 0, 100, 255, 0);
    mFeedback = (int) map(yp, 0, 100, -100, 100);

    // enable these for some more fun :)
    // mSampleRate = map(yp,0,100,1,512);
    // mStages = (int) ( 2*map(yp,0,100,1,12));


    //init
    float gain = 0, fbout = 0;
    float lfoskip = mFreq * 2 * PI / mSampleRate;
    float phase = mPhase * PI / 180;

    float[] old = new float[mStages];
    for ( int j = 0; j < mStages; j++) { 
      old[j] = 0.0;
    }

    /* EffectPhaser::ProcessBlock */
    int skipcount = 0;
    //start the show :)  
    PImage result = createImage(img.width, img.height, RGB);
    img.loadPixels(); 
    result.loadPixels();

    float[] rgb = new float[3];
    for ( int i = 0, l = img.pixels.length; i<l; i++ ) { 
      color c = img.pixels[i];
      rgb[0] = map(red(c), 0, 255, 0, 1);
      rgb[1] = map(green(c), 0, 255, 0, 1);
      rgb[2] = map(blue(c), 0, 255, 0, 1);
      for ( int ci = 0; ci < 3; ci++) { 
        float in = rgb[ci];
        float m = in + fbout * mFeedback / 100;
        if ( (( skipcount++) % lfoskipsamples ) == 0 ) { //recomopute lfo
          gain = (1.0 + cos(skipcount * lfoskip + phase)) / 2.0; //compute sine between 0 and 1
          gain = exp(gain * phaserlfoshape) / exp(phaserlfoshape); // change lfo shape
          gain = 1.0 - gain / 255.0 * mDepth; // attenuate the lfo
        }
        //phasing routine
        for ( int j = 0; j<mStages; j++) {
          float tmp = old[j];
          old[j] = gain * tmp + m;
          m = tmp - gain * old[j];
        }
        fbout = m;
        rgb[ci] = (float) (( m * mDryWet + in * (255-mDryWet)) / 255);
      }
      color rc = color(
      map(rgb[0], 0, 1, 0, 255), 
      map(rgb[1], 0, 1, 0, 255), 
      map(rgb[2], 0, 1, 0, 255));
      result.pixels[i] = rc;
    }
    result.updatePixels();
    return result;
  }
}



class AuEcho implements IEffect { 
  PImage run(PImage img, float xp, float yp) { 
    PImage result = createImage(img.width, img.height, RGB);
    float _delay = map(xp, 0, 100, 0.001, 1.0);
    float decay = map(yp, 0, 100, 0.0, 1.0);
    int delay = (int)(img.pixels.length * _delay);
    color[] history = new color[img.pixels.length];
    int blendMode = BLEND;
    img.loadPixels(); 
    result.loadPixels();
    for ( int i = 0, l = img.pixels.length; i<l; i++) {
      history[i] = img.pixels[i];
    }
    for ( int i = 0, l = img.pixels.length; i<l; i++) {
      int fromPos = i-delay < 0 ? l-abs(i-delay) : i-delay;
      color fromColor = history[fromPos];
      float r = red(fromColor) * decay;
      float g = green(fromColor) * decay;
      float b = blue(fromColor) * decay;
      color origColor = history[i];
      color toColor = color(
      r = r + red(origColor) > 255 ? r + red(origColor) - 255 : r + red(origColor), // simulate overflow ;) duz java do dis?
      g = g + green(origColor) > 255 ? g + green(origColor) - 255 : g + green(origColor), 
      b = b + blue(origColor) > 255 ? b + blue(origColor) - 255 : b + blue(origColor)  );

      result.pixels[i] = history[i] = toColor; //blendColor(origColor, toColor, blendMode);
    }  
    return result;
  }
}


class AuWahwah implements IEffect { 
  PImage run(PImage img, float xp, float yp) { 
    float phase, lfoskip, xn1, xn2, yn1, yn2, b0, b1, b2, a0, a1, a2, freqofs, freq, freqoff, startphase, res, depth; 
    float mCurRate = 0.4, skipcount = 0;
    int lfoskipsamples = 0;
    float frequency, omega, sn, cs, alpha;
    float in, out;
    float val;
    PImage result = createImage(img.width, img.height, RGB);
    freq = 1.5;
    startphase = 0.2;
    depth = 0.8;
    freqofs = 0.9;
    res = 12.5;
    lfoskip = freq * 2 * PI / mCurRate;
    skipcount = xn1 = xn2 = yn1 = yn2 = b0 = b1 = b2 = a0 = a1 = a2 = 0;
    phase = startphase;
    res=map(xp, 0, 100, 1, 100);
    depth=map(yp, 0, 100, 0, 1);
    img.loadPixels(); 
    result.loadPixels();
    float[] rgb = new float[3];
    for ( int i = 0, len = img.pixels.length; i < len; i++) { 
      rgb[0] = red(img.pixels[i]); 
      rgb[1] = green(img.pixels[i]);
      rgb[2] = blue(img.pixels[i]);
      for ( int ri = 0; ri < 3; ri++ ) { 
        in = map(rgb[ri], 0, 255, 0, 1);
        if (true || (skipcount++) % lfoskipsamples == 0) { 
          frequency = (1+cos(skipcount * lfoskip + phase ))/2;
          frequency = frequency * depth * (1-freqofs) + freqofs;
          frequency = exp((frequency - 1) * 6 );
          omega = PI * frequency;
          sn = sin(omega);
          cs = cos(omega);
          alpha = sn/(2*res);
          b0 = (1-cs) /2;
          b1 = 1 - cs;
          b2 = (1-cs)/2;
          a0 = 1 + alpha;
          a1 = -2 * cs;
          a2 = 1 - alpha;
        }
        out = ( b0 * in + b1 * xn1 + b2 * xn2 - a1 * yn1 - a2 * yn2 ) / a0;
        xn2 = xn1;
        xn1 = in;
        yn2 = yn1;
        yn1 = out;
        rgb[ri] = map(out, 0, 1, 0, 255);
      }
      result.pixels[i] = color(rgb[0], rgb[1], rgb[2]);
    }
    result.updatePixels();
    return result;
  }
}

