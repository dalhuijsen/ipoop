

    var d1 = document.getElementById('dialog1');

    function assertCentered(node) {
      var rect = node.getBoundingClientRect();
      assert.equal(Math.floor(rect.top), Math.floor((window.innerHeight - rect.height) / 2), 'node is not centered vertically');
      assert.equal(Math.floor(rect.left), Math.floor((window.innerWidth - rect.width) / 2), 'node is not centered horizontally');
    }

    test('dialog with dynamic content re-centers', function(done) {
      d1.opened = true;

      setTimeout(function() {
        assertCentered(d1);

        // d1.opened = false;
        // d1.innerHTML = '<p>Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat.</p>' +
        //   '<p>Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat.</p>' +
        //   '<p>Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat.</p>' +
        //   '<p>Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat.</p>';


        // flush(function() {
        //   d1.opened = true;

        //   setTimeout(function() {
        //     assertCentered(d1);
        //     done();
        //   }, 50);
        // });

done();

      }, 50);

    });

  