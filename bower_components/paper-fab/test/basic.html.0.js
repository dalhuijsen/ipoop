

    var f1 = document.getElementById('fab1');

    function centerOf(element) {
      var rect = element.getBoundingClientRect();
      return {left: rect.left + rect.width / 2, top: rect.top + rect.height / 2};
    }

    test('renders correctly independent of line height', function() {
      assert.deepEqual(centerOf(f1.$.icon), centerOf(f1));
    });

  