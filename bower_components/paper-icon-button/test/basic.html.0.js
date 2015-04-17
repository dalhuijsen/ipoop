

    var b1 = document.getElementById('button1');

    function centerOf(element) {
      var rect = element.getBoundingClientRect();
      return {left: rect.left + rect.width / 2, top: rect.top + rect.height / 2};
    }

    function approxEqual(p1, p2) {
      return Math.round(p1.left) == Math.round(p2.left) && Math.round(p1.top) == Math.round(p2.top);
    }

    test('renders correctly independent of font size', function() {
      assert.ok(approxEqual(centerOf(b1.$.icon), centerOf(b1)));
    });

  