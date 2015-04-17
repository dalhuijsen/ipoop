
    document.querySelector('button').addEventListener('click', function() {
      var spinners = document.querySelectorAll('paper-spinner');
      Array.prototype.forEach.call(spinners, function(spinner) {
        spinner.active = !spinner.active;
      });
    });
  