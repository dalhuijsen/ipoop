
assertInterpolation({
  property: 'top',
  from: '-10px',
  to: '10px'
}, [
  {at: -0.3, is: '-16px'},
  {at: 0, is: '-10px'},
  {at: 0.5, is: '0px'},
  {at: 1, is: '10px'},
  {at: 1.5, is: '20px'}
]);
