
assertInterpolation({
  property: 'margin-left',
  from: 'calc(50% - 25px)',
  to: 'calc(100% - 10px)'
}, [
  {at: -0.25, is: '-10px'},
  {at: 0, is: '0px'},
  {at: 0.25, is: '10px'},
  {at: 0.5, is: '20px'},
  {at: 0.75, is: '30px'},
  {at: 1, is: '40px'},
  {at: 1.25, is: '50px'}
]);

assertInterpolation({
  property: 'text-indent',
  from: 'calc(50% - 25px)',
  to: 'calc(100% - 10px)'
}, [
  {at: -0.25, is: 'calc(((50% - 25px) * 1.25) + ((100% - 10px) * -0.25))'},
  {at: 0, is: 'calc(50% - 25px)'},
  {at: 0.25, is: 'calc(((50% - 25px) * 0.75) + ((100% - 10px) * 0.25))'},
  {at: 0.5, is: 'calc(((50% - 25px) * 0.5) + ((100% - 10px) * 0.5))'},
  {at: 0.75, is: 'calc(((50% - 25px) * 0.25) + ((100% - 10px) * 0.75))'},
  {at: 1, is: 'calc(100% - 10px)'},
  {at: 1.25, is: 'calc(((50% - 25px) * -0.25) + ((100% - 10px) * 1.25))'}
]);

assertInterpolation({
  property: 'text-indent',
  from: '0em',
  to: '100px'
}, [
  {at: -0.25, is: '-25px'},
  {at: 0, is: '0em'},
  {at: 0.25, is: '25px'},
  {at: 0.5, is: '50px'},
  {at: 0.75, is: '75px'},
  {at: 1, is: '100px'},
  {at: 1.25, is: '125px'}
]);

assertInterpolation({
  property: 'text-indent',
  from: '0%',
  to: '100px'
}, [
  {at: -0.25, is: 'calc(0% + -25px)'},
  {at: 0, is: '0%'},
  {at: 0.25, is: 'calc(0% + 25px)'},
  {at: 0.5, is: 'calc(0% + 50px)'},
  {at: 0.75, is: 'calc(0% + 75px)'},
  {at: 1, is: '100px'},
  {at: 1.25, is: 'calc(0% + 125px)'}
]);

