

  var fake = new Fake();

  test('change event bubbles', function(done) {
    var node = document.getElementById('default');
    var changeAction = function() {
      done();
      node.removeEventListener('change', changeAction);
    };
    node.addEventListener('change', changeAction);
    var input = node.shadowRoot.querySelector('input');
    var e = new Event('change', {
      bubbles: true
    });
    input.dispatchEvent(e);
  });

  test('cannot tap on disabled input', function(done) {
    var node = document.getElementById('disabled');
    fake.downOnNode(node);
    fake.upOnNode(node);
    setTimeout(function() {
      assert.notStrictEqual(activeElement(), node.shadowRoot.querySelector('input'));
      done();
    }, 10);
  });

  