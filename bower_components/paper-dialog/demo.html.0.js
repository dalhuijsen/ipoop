


  var scope = document.querySelector('template[is=auto-binding]');

  scope.toggleDialog1 = function(e) {
    if (e.target.localName != 'button') {
      return;
    }
    var d = e.target.nextElementSibling;
    if (!d) {
      return;
    }
    d.toggle();
  };

  scope.transitions = [
    'core-transition-center',
    'core-transition-top',
    'core-transition-bottom',
    'core-transition-left',
    'core-transition-right'
  ];

  scope.toggleDialog2 = function(e) {
    if (e.target.localName != 'button') {
      return;
    }
    scope.transition = e.target.getAttribute('transition');
    document.getElementById('dialog2').toggle();
  };

