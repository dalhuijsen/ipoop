
assertInterpolation({
  property: 'min-height',
  from: '0px',
  to: '100px'
}, [
  {at: -0.5, is: '0'}, // CSS min-height can't be negative.
  {at: 0, is: '0'},
  {at: 0.3, is: '30px'},
  {at: 0.6, is: '60px'},
  {at: 1, is: '100px'},
  {at: 1.5, is: '150px'}
]);
