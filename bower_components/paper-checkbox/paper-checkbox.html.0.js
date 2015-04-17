

    Polymer('paper-checkbox', {

      /**
       * Fired when the checked state changes due to user interaction.
       *
       * @event change
       */

      /**
       * Fired when the checked state changes.
       *
       * @event core-change
       */

      toggles: true,

      checkedChanged: function () {
        this.setAttribute('aria-checked', this.checked ? 'true' : 'false');
        this.fire('core-change');
      }

    });

  