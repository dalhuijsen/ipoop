
assertInterpolation({
  property: 'border-spacing',
  from: '0px',
  to: '10px'
}, [
  {at: -0.3, is: '0px 0px'}, // Can't be negative.
  {at: 0, is: '0px 0px'},
  {at: 0.3, is: '3px 3px'},
  {at: 0.6, is: '6px 6px'},
  {at: 1, is: '10px 10px'},
  {at: 1.5, is: '15px 15px'}
]);
assertInterpolation({
  property: '-webkit-border-horizontal-spacing',
  from: '0px',
  to: '10px'
}, [
  {at: -0.3, is: '0px'}, // Can't be negative.
  {at: 0, is: '0px'},
  {at: 0.3, is: '3px'},
  {at: 0.6, is: '6px'},
  {at: 1, is: '10px'},
  {at: 1.5, is: '15px'}
]);
assertInterpolation({
  property: '-webkit-border-vertical-spacing',
  from: '0px',
  to: '10px'
}, [
  {at: -0.3, is: '0px'}, // Can't be negative.
  {at: 0, is: '0px'},
  {at: 0.3, is: '3px'},
  {at: 0.6, is: '6px'},
  {at: 1, is: '10px'},
  {at: 1.5, is: '15px'}
]);
