

    function assertPosition(dropdown, trigger) {
      var dr = dropdown.getBoundingClientRect();
      var tr = trigger.getBoundingClientRect();

      if (dropdown.halign === 'left') {
        assert.equal(dr.left, tr.left);
      } else {
        assert.equal(dr.right, tr.right);
      }

      if (dropdown.valign === 'top') {
        assert.equal(dr.top, tr.top);
      } else {
        assert.equal(dr.bottom, tr.bottom);
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
    d1.relatedTarget = t1;

    var d2 = document.getElementById('dropdown2');
    var t2 = document.getElementById('trigger2');
    d2.relatedTarget = t2;

    var d3 = document.getElementById('dropdown3');
    var t3 = document.getElementById('trigger3');
    d3.relatedTarget = t3;

    test('default', function(done) {
      d1.opened = true;
      flushLayoutAndRender(function() {
        assertPosition(d1, t1);
        done();
      });
    });

    test('bottom alignment', function(done) {
      d2.valign = 'bottom';
      d2.opened = true;
      flushLayoutAndRender(function() {
        assertPosition(d2, t2);
        done();
      });
    });

    test('right alignment', function(done) {
      d3.halign = 'right';
      d3.opened = true;
      flushLayoutAndRender(function() {
        assertPosition(d3, t3);
        done();
      });
    });

  