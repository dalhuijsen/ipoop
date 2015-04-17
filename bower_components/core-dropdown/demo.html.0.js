
    Polymer({
      toggle: function() {
        if (!this.dropdown) {
          this.dropdown = this.querySelector('core-dropdown');
        }
        this.dropdown && this.dropdown.toggle();
      }
    });
  