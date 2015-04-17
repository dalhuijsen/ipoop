

    var f1 = document.getElementById('fab1');
    var f2 = document.getElementById('fab2');
    var f3 = document.getElementById('fab3');

    test('aria role is a button', function() {
      assert.strictEqual(f1.getAttribute('role'), 'button');
    });

    test('aria-disabled is set', function(done) {
      assert.ok(f2.hasAttribute('aria-disabled'));
      f2.removeAttribute('disabled');
      flush(function() {
        assert.ok(!f2.hasAttribute('aria-disabled'));
        done();
      });
    });

    test('aria-label is set', function() {
      assert.strictEqual(f1.getAttribute('aria-label'), 'add');
    });

    test('user-defined aria-label is preserved', function(done) {
      assert.strictEqual(f3.getAttribute('aria-label'), 'custom');
      f3.icon = 'arrow-forward';
      flush(function() {
        assert.strictEqual(f3.getAttribute('aria-label'), 'custom');
        done();
      });
    });

  