
    var player;

    document.body.addEventListener('core-animation-finish', function(e) {
      console.log('core-animation-finish');
      if (player) {
        player.cancel();
        player = null;
        target.querySelector('span').textContent = 'polymer';
      }
    });

    var customAnimationFn = function(timeFraction, target) {
      // var colors = [
      //   '#db4437',
      //   '#ff9800',
      //   '#ffeb3b',
      //   '#0f9d58',
      //   '#4285f4',
      //   '#3f51b5',
      //   '#9c27b0'
      // ];
      target.querySelector('span').textContent = timeFraction;
    };


    function clickAction(e) {
      var t = e.target;
      if (e.target.localName !== 'button') {
        return;
      }

      if (player) {
        player.cancel();
      }

      var a = t.querySelector('core-animation,core-animation-group');
      if (a.id === 'custom-animation') {
        a.customEffect = customAnimationFn;
      }

      a.target = document.getElementById('target');
      player = a.play();
    }
  