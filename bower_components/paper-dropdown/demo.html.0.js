
    Polymer({
      toggle: function() {
        if (!this.dropdown) {
          this.dropdown = this.querySelector('paper-dropdown');
        }
        this.dropdown && this.dropdown.toggle();
      }
    });
  