
assertInterpolation({
  property: 'border-radius',
  from: '10px',
  to: '50px'
}, [
  {at: -0.3, is: '0px'}, // CSS border-radius can't be negative.
  {at: 0, is: '10px'},
  {at: 0.3, is: '22px'},
  {at: 0.6, is: '34px'},
  {at: 1, is: '50px'},
  {at: 1.5, is: '70px'},
]);
assertInterpolation({
  property: 'border-radius',
  from: '10px',
  to: '100%'
}, [
  // These expectations are expected to fail on the current animation engine
  // with different (but equivalent) calc expressions.
  {at: -0.3, is: 'calc(13px + -30%)'},
  {at: 0, is: '10px'},
  {at: 0.3, is: 'calc(7px + 30%)'},
  {at: 0.6, is: 'calc(4px + 60%)'},
  {at: 1, is: '100%'},
  {at: 1.5, is: 'calc(-5px + 150%)'},
]);
