

  Polymer({

    publish: {
      /**
       * The id of the textinput or textarea that should be monitored.
       *
       * @attribute target
       * @type string
       * @default null
       */
      target: null,

      /**
       * If false, don't show the character counter. Used in conjunction with
       * `paper-input-decorator's` `error` field.
       *
       * @attribute showCounter
       * @type boolean
       * @default true
       */
      showCounter: true
    },

    /* Number of characters in the current input */
    _charCount: 0,

    /* Equal to the target element's maxLength attribute. */
    _maxChars: 0,

    /* True if the number of characters in the input exceeds _maxChars */
    _isCounterInvalid: false,

    ready: function() {
      if (!this.target)
        return;
      var targetElement = document.getElementById(this.target);
      this._maxChars = targetElement.maxLength;
      targetElement.addEventListener('input', this.inputAction.bind(this));
		},

    inputAction: function(e) {
      this._charCount = e.target.value.length;
      this._isCounterInvalid = this._maxChars && this._charCount >= this._maxChars;
    },

    _isCounterInvalidChanged: function() {
      debugger
      this.classList.toggle('invalid', this._isCounterInvalid);
      this.fire('char-counter-error',
                {"hasError": this._isCounterInvalid,
                 "hideErrorIcon": this.showCounter});
    }
  });

