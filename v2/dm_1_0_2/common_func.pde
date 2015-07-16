/* util */
void setupEnumeratedTypes() { 
  Class[] types = this.getClass().getDeclaredClasses();
  state.allTypes = types;
  /* Dynamic class loader with help of http://stackoverflow.com/users/873165/kevin-workman */
  //enumerate castable types
  for (int i = 0, l = types.length; i<l; i++) { 
    Class c = types[i];
    if ( !c.isInterface() && IEffect.class.isAssignableFrom(c) ) { 
      println("adding "+c.toString());
      state.fxTypes.put(c.getSimpleName(), c);
    }
  }
  println(this.getClass().getName());
  Class<?> pc = this.getClass();
  for ( String s : state.fxTypes.keySet () ) { 
    Class c = state.fxTypes.get(s);
    try { 
      java.lang.reflect.Constructor ctor = c.getDeclaredConstructor(pc);// getConstructors();
      ctor.setAccessible(true);
      try { 
        Object o = ctor.newInstance(this);
        IEffect f = (IEffect)o;
        state.fx.put(s, f);
      } 
      catch(Exception e) { 
        println("failed to instantiate on "+s);
      }
    } 
    catch( Exception e) { 
      println("failed to get ctor on "+s);//      e.printStackTrace();
    }
  }
  /*
  if ( state.allButtons == null || state.allButtons.size() < 1 ) { 
   HashMap<String, ArrayList<IMenuRunnable>> cats = new HashMap<String, ArrayList<IMenuRunnable>>();
   for (String s : state.runnables.keySet () ) { 
   IMenuRunnable imr = state.runnables.get(s);
   if ( imr == null ) continue;
   //String s = imr.getClass().toString();
   String cn = imr.group();
   if ( cats.get(cn) == null ) { 
   cats.put(cn, new ArrayList<IMenuRunnable>());
   }
   cats.get(cn).add(imr);
   } 
   // for (String kn : cats.keySet () ) { 
   //   println(kn);
   // }
   state.allButtons = cats;
   }
   */
}



