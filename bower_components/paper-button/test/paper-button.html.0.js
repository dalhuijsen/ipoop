

    var fake = new Fake();

    var b1 = document.getElementById('button1');

    test('can set raised imperatively', function(done) {
      assert.ok(!b1.shadowRoot.querySelector('paper-shadow'));
      b1.raised = true;
      flush(function() {
        setTimeout(function() {
          var shadow = b1.shadowRoot.querySelector('paper-shadow');
          assert.ok(shadow);
          done();
        }, 600);
      });
    });

    test('can set noink dynamically', function(done) {
      var button = document.getElementById('button2');
      button.lastEvent = {x: 100, y: 100};
      button.$.ripple = {
        downAction: function() {
          assert.ok(false);
        },
        upAction: function() {
          assert.ok(false);
        }
      };
      button.setAttribute('noink', '');
      fake.downOnNode(button);
      fake.upOnNode(button);
      // would throw if it tries to ripple
      setTimeout(done, 10);
    });

    suite('a11y', function() {

      test('aria role is a button', function() {
        assert.strictEqual('button', b1.getAttribute('role'));
      });

      test('aria-disabled is set', function(done) {
        var button = document.getElementById('disabled');
        assert.ok(button.hasAttribute('aria-disabled'));
        button.removeAttribute('disabled');
        flush(function() {
          assert.ok(!button.hasAttribute('aria-disabled'));
          done();
        });
      });

      test('space triggers the button', function() {
        var ev = new CustomEvent('keydown', {detail: {key: 'space'}});
        var sawClick = false;
        function clickListener() {
          sawClick = true;
        }
        b1.addEventListener('click', clickListener);
        b1.dispatchEvent(ev);
        assert.ok(sawClick);
        b1.removeEventListener(clickListener);
      });

      test('enter triggers the button', function() {
        var ev = new CustomEvent('keydown', {detail: {key: 'enter'}});
        var sawClick = false;
        function clickListener() {
          sawClick = true;
        }
        b1.addEventListener('click', clickListener);
        b1.dispatchEvent(ev);
        assert.ok(sawClick);
        b1.removeEventListener(clickListener);
      });

    });

  