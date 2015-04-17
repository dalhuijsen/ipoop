

    function approxEqual(a, b) {
      return assert.equal(Math.round(a), Math.round(b));
    }

    function assertPosition(dropdown, trigger) {
      var dr = dropdown.getBoundingClientRect();
      var tr = trigger.getBoundingClientRect();

      if (dropdown.halign === 'left') {
        approxEqual(dr.left, tr.left);
      } else {
        approxEqual(dr.right, tr.right);
      }

      if (dropdown.valign === 'top') {
        approxEqual(dr.top, tr.top);
      } else {
        approxEqual(dr.bottom, tr.bottom);
      }
    };

    function flushLayoutAndRender(callback) {
      flush(function() {
        document.body.offsetTop;
        requestAnimationFrame(function() {
          callback();
        });
      });
    }

    var d1 = document.getElementById('dropdown1');
    var t1 = document.getElementById('trigger1');

    var d2 = document.getElementById('dropdown2');
    var t2 = document.getElementById('trigger2');

    var d3 = document.getElementById('dropdown3');
    var t3 = document.getElementById('trigger3');

    var d4 = document.getElementById('dropdown4');
    var t4 = document.getElementById('trigger4');

    var d5 = document.getElementById('dropdown5');
    var t5 = document.getElementById('trigger5');

    var d6 = document.getElementById('dropdown6');
    var t6 = document.getElementById('trigger6');

    test('default', function(done) {
      d1.opened = true;
      flushLayoutAndRender(function() {
        assertPosition(d1, t1);
        done();
      });
    });

    test('bottom alignment', function(done) {
      d2.opened = true;
      flushLayoutAndRender(function() {
        assertPosition(d2, t2);
        done();
      });
    });

    test('right alignment', function(done) {
      d3.opened = true;
      flushLayoutAndRender(function() {
        assertPosition(d3, t3);
        done();
      });
    });

    test('layered', function(done) {
      d4.opened = true;
      flushLayoutAndRender(function() {
        assertPosition(d4, t4);
        done();
      });
    });

    test('layered, bottom alignment', function(done) {
      d5.opened = true;
      flushLayoutAndRender(function() {
        assertPosition(d5, t5);
        done();
      });
    });

    test('layered, right alignment', function(done) {
      d6.opened = true;
      flushLayoutAndRender(function() {
        assertPosition(d6, t6);
        done();
      });
    });

  