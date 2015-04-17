
assertInterpolation({
  property: 'padding',
  from: '0px',
  to: '10px'
}, [
  {at: -0.3, is: '0px'}, // CSS padding can't be negative.
  {at: 0, is: '0px'},
  {at: 0.3, is: '3px'},
  {at: 0.6, is: '6px'},
  {at: 1, is: '10px'},
  {at: 1.5, is: '15px'}
]);
